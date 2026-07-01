#include "main_window.hpp"
#include "dialog/dict_selection_dialog.hpp"
#include "dialog/find_replace_dialog.hpp"
#include "dialog/first_run_dialog.hpp"
#include "dialog/merge_dialog.hpp"
#include "dialog/spell_context_menu.hpp"
#include "dialog/translator_settings_dialog.hpp"
#include "editor/grammar_checker.hpp"
#include "highlighter/editor_highlighter.hpp"
#include "highlighter/glossary_highlighter.hpp"
#include "highlighter/topic_highlighter.hpp"
#include "model/dict_document.hpp"
#include "model/plugin_document.hpp"
#include "model/table_builder.hpp"
#include "model/yaml_document.hpp"
#include "translator/ctranslate2_translator.hpp"
#include "utility/display_name.hpp"
#include "view/annotations_view.hpp"
#include "view/book_preview_view.hpp"
#include "view/editor_view.hpp"
#include "view/filter_tree_view.hpp"
#include "view/history_view.hpp"
#include "view/log_view.hpp"
#include "view/record_table_view.hpp"
#include "view/sidebar_view.hpp"
#include "view/status_filter_view.hpp"
#include "view/translation_suggestion_view.hpp"
#include "view/validation_view.hpp"
#include <io/dict_writer.hpp>
#include <merger/dict_merger.hpp>
#include <utility/string_utils.hpp>
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <unordered_map>
#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextDocument>
#include <QTextOption>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

main_window_t::main_window_t(QWidget * parent)
    : QMainWindow(parent)
    , m_session(m_current_codepage)
    , m_editor_controller(m_edit_history, m_byte_limit_validator, m_glossary)
{
	setWindowTitle("yTranslator");
	resize(1280, 720);
	setMinimumSize(800, 600);

	m_type_filter = {
		tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
		tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
		tools_t::rec_type_t::indx, tools_t::rec_type_t::sctx,
	};

	setup_menu_bar();
	setup_toolbar();
	setup_central_widget();
	setup_sidebar();
	setup_editor_panel();
	setup_status_bar();
	setup_table_display();

	connect_menu_signals();
	connect_sidebar_signals();
	connect_editor_signals();
	connect_search_signals();

	load_config();

	bool first_run = m_settings.native_language().empty();

	if (first_run)
	{
		first_run_dialog_t dialog(this);
		if (dialog.exec() == QDialog::Accepted)
		{
			const auto native = dialog.selected_native_language();
			const auto foreign = dialog.selected_foreign_language();

			m_settings.set_native_language(native);
			m_settings.set_foreign_language(foreign);
			m_settings.set_native_tag(native);
			m_settings.set_foreign_tag(foreign);

			int encoding_index = 2;
			if (native == "PL")
				encoding_index = 0;
			else if (native == "RU")
				encoding_index = 1;

			m_settings.set_encoding_index(encoding_index);
			on_encoding_changed(encoding_index);

			save_config();
		}
	}
}

void main_window_t::set_unsaved_changes(bool dirty)
{
	if (m_has_unsaved_changes == dirty)
		return;

	m_has_unsaved_changes = dirty;
	setWindowTitle(m_has_unsaved_changes ? "yTranslator *" : "yTranslator");
}

void main_window_t::on_save()
{
	commit_current_edit();

	if (!m_active_doc)
		return;

	if (!m_active_doc->is_dirty())
		return;

	m_active_doc->save();

	update_sidebar_item(m_active_doc->path());

	m_log_view->append_log("save", "saved \"" + m_active_doc->path() + "\"\r\n");

	if (!m_session.has_any_unsaved())
		set_unsaved_changes(false);
}

void main_window_t::on_save_all()
{
	commit_current_edit();

	std::string log_msg;
	for (auto * doc : m_session.all_dirty())
		log_msg += "saved \"" + doc->path() + "\"\r\n";

	m_session.save_all();

	for (auto * doc : m_session.all())
		update_sidebar_item(doc->path());

	if (!log_msg.empty())
		m_log_view->append_log("save all", log_msg);

	if (!m_session.has_any_unsaved())
		set_unsaved_changes(false);
}

void main_window_t::on_merge()
{
	const auto all_dicts = m_session.all_dicts();
	if (all_dicts.size() < 2)
	{
		QMessageBox::information(this, "Merge", "At least 2 dictionaries must be loaded to merge.");
		return;
	}

	std::vector<merge_dialog_t::dict_entry_t> loaded_dicts;
	for (const auto * dict_doc : all_dicts)
	{
		auto filename = std::string(string_utils::extract_filename(dict_doc->path()));
		loaded_dicts.push_back({ filename, dict_doc->path(), dict_doc->kind() });
	}

	merge_dialog_t dialog(loaded_dicts, this);
	if (dialog.exec() != QDialog::Accepted)
		return;

	const auto selected_paths = dialog.selected_paths();
	if (selected_paths.size() < 2)
		return;

	tools_t::reset_log();

	dict_merger_t merger(selected_paths);
	const auto & merged_dict = merger.get_dict();

	const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";
	QDir().mkpath(QString::fromStdString(workspace_dir));

	const auto output_path =
	    workspace_dir + "Merged_" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss").toStdString() + ".json";

	dict_writer_t::write(merged_dict, output_path);

	const auto log_text = tools_t::get_log();
	m_log_view->append_log("merge", log_text);
	m_record_tabs->setCurrentWidget(m_log_view);

	scan_workspace();

	const auto norm_output = string_utils::normalize_path(output_path);
	auto * doc = m_session.find(norm_output);
	if (doc)
	{
		switch_document(doc);
		rebuild_sidebar();
	}
}

void main_window_t::on_find()
{
	m_search_field->setFocus();
	m_search_field->selectAll();
}

void main_window_t::on_escape()
{
	if (!m_search_field->text().isEmpty())
		m_search_field->clear();
}

void main_window_t::on_search_changed(const QString & text)
{
	m_search_query = text;

	row_filter_t::config_t cfg;
	cfg.query = text.toStdString();
	cfg.case_sensitive = m_case_sensitive_check && m_case_sensitive_check->isChecked();
	cfg.regex_mode = m_regex_check && m_regex_check->isChecked();
	cfg.columns.clear();
	if (m_search_col_key->isChecked())
		cfg.columns.insert(search_column_t::key);
	if (m_search_col_original->isChecked())
		cfg.columns.insert(search_column_t::original);
	if (m_search_col_translation->isChecked())
		cfg.columns.insert(search_column_t::translation);
	m_row_filter.set_config(cfg);

	rebuild_table();
}

void main_window_t::on_case_sensitive_changed(int /*state*/)
{
	on_search_changed(m_search_query);
}

void main_window_t::on_filters_changed()
{
	m_type_filter = m_filter_tree_view->get_active_types();
	m_type_filter_solo = m_filter_tree_view->has_sub_type_filter();
	rebuild_table();
}

void main_window_t::on_status_filters_changed()
{
	m_status_filter = m_status_filter_view->get_active_statuses();
	rebuild_table();
}

void main_window_t::clear_editor_panels()
{
	m_editor_view->original_view()->clear();
	m_editor_view->translation_editor()->clear();
	m_editor_view->clear_details();
	m_validation_view->clear();
	m_annotations_view->clear();
	m_history_view->clear();
	m_book_preview_view->clear();
}

void main_window_t::switch_document(document_t * new_doc)
{
	commit_current_edit();
	save_current_filter_state();

	m_active_doc = new_doc;
	m_editor_controller.set_current_row(-1);

	if (!m_active_doc)
	{
		rebuild_table();
		clear_editor_panels();
		return;
	}

	restore_filter_state(m_active_doc->path());

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (dict_doc)
	{
		m_filter_tree_view->set_display_mode(filter_tree_view_t::display_mode_t::full);
		if (m_session.dict_version() != m_last_annotation_version)
		{
			rebuild_annotations();
			m_last_annotation_version = m_session.dict_version();
		}
	}
	else if (dynamic_cast<plugin_document_t *>(m_active_doc))
	{
		m_filter_tree_view->set_display_mode(filter_tree_view_t::display_mode_t::empty);
		m_filter_tree_view->setEnabled(false);
	}
	else
	{
		m_filter_tree_view->set_display_mode(filter_tree_view_t::display_mode_t::all_only);
	}

	rebuild_table();
	clear_editor_panels();
}

void main_window_t::rebuild_table()
{
	if (!m_table_model)
		return;

	if (!m_active_doc)
	{
		m_table_display->clear();
		m_editor_controller.set_current_row(-1);
		clear_editor_panels();
		return;
	}

	if (dynamic_cast<plugin_document_t *>(m_active_doc))
	{
		m_table_display->clear();
		m_editor_controller.set_current_row(-1);
		clear_editor_panels();
		return;
	}

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (!dict_doc)
	{
		rebuild_table_yaml(m_active_doc);
		return;
	}

	rebuild_table_dict(dict_doc);
}

void main_window_t::rebuild_table_yaml(document_t * target_doc)
{
	const auto raw_rows = target_doc->build_rows();

	std::map<status_t, size_t> total_status_counts;
	std::map<status_t, size_t> filtered_status_counts;

	for (const auto & row : raw_rows)
	{
		total_status_counts[row.status]++;

		if (m_row_filter.has_query() && !m_row_filter.matches(row))
			continue;

		filtered_status_counts[row.status]++;
	}

	std::vector<table_row_t> rows;
	for (const auto & row : raw_rows)
	{
		if (!m_status_filter.empty() && m_status_filter.count(row.status) == 0)
			continue;

		if (m_row_filter.has_query() && !m_row_filter.matches(row))
			continue;

		rows.push_back(row);
	}

	int total = target_doc->total_count();
	int translated = target_doc->translated_count();
	m_table_display->apply_yaml(
	    std::move(rows), total, translated, target_doc->path(), filtered_status_counts, total_status_counts);
	m_editor_controller.set_current_row(-1);
	clear_editor_panels();
}

void main_window_t::rebuild_table_dict(dict_document_t * dict_doc)
{
	const table_filter_params_t filter_params {
		m_type_filter, m_filter_tree_view->get_active_sub_types(), m_status_filter, m_row_filter, m_type_filter_solo
	};

	auto result = build_filtered_rows(dict_doc->data(), filter_params);

	m_table_display->apply(std::move(result), dict_doc->path(), dict_doc->kind());
	m_editor_controller.set_current_row(-1);
	clear_editor_panels();
}

void main_window_t::on_row_selected(int row)
{
	if (row == m_editor_controller.current_row())
		return;

	commit_current_edit();
	load_record(row);
}

void main_window_t::on_translation_changed()
{
	if (m_editor_controller.is_loading())
		return;

	if (m_editor_controller.current_row() < 0)
		return;

	if (!m_active_doc)
		return;

	auto * yaml_doc = dynamic_cast<yaml_document_t *>(m_active_doc);
	if (yaml_doc)
	{
		if (!yaml_doc->is_dirty())
		{
			yaml_doc->set_dirty(true);
			update_sidebar_item(yaml_doc->path());
			set_unsaved_changes(true);
		}

		return;
	}

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (dict_doc)
	{
		if (!dict_doc->is_dirty())
		{
			dict_doc->set_dirty(true);
			update_sidebar_item(dict_doc->path());
		}
	}

	set_unsaved_changes(true);
	update_validation();

	if (m_editor_controller.current_row() < 0)
		return;

	const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
	if (!row_data)
		return;

	apply_translation_highlights(row_data);

	if (row_data->type == tools_t::rec_type_t::text)
	{
		const auto translation_text = m_editor_view->translation_editor()->toPlainText().toStdString();
		m_book_preview_view->set_html(row_data->old_text, translation_text);
	}
}

void main_window_t::apply_translation_highlights(const table_row_t * row_data)
{
	const auto annotations = m_glossary.annotate(row_data->old_text, row_data->type);
	const auto current_text = m_editor_view->translation_editor()->toPlainText().toLower();

	const highlight_config_t config { &annotations, false, highlight_sort_policy_t::hyperlink_first };
	auto highlights = find_annotation_highlights(current_text, config);

	m_extra_sel_translation.annotations = build_highlight_selections(m_editor_view->translation_editor(), highlights);

	m_extra_sel_translation.grammar = m_grammar_check->isChecked()
	                                      ? m_grammar_checker.check(m_editor_view->translation_editor(), row_data->type)
	                                      : QList<QTextEdit::ExtraSelection> {};
	apply_extra_selections(m_editor_view->translation_editor(), m_extra_sel_translation);
}

void main_window_t::commit_current_edit()
{
	if (m_editor_controller.current_row() < 0)
		return;

	if (m_editor_controller.is_loading())
		return;

	if (!m_editor_view)
		return;

	if (!m_active_doc)
		return;

	const auto & current_text = m_editor_view->translation_editor()->toPlainText();
	if (current_text == m_editor_controller.loaded_text())
		return;

	const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
	if (!row_data)
		return;

	std::string new_text_str;
	if (m_editor_view->has_script_template())
	{
		new_text_str = m_editor_view->reconstruct_script_line();

		const auto lines = current_text.split('\n');
		const size_t slot_count = m_editor_view->script_slot_count();

		if (static_cast<size_t>(lines.size()) != slot_count)
		{
			statusBar()->showMessage(
			    QString("Warning: expected %1 strings, got %2").arg(slot_count).arg(lines.size()), 5000);
		}
	}
	else
	{
		new_text_str = current_text.toStdString();
	}

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (dict_doc)
	{
		commit_dict_edit(dict_doc, row_data, new_text_str);
		return;
	}

	commit_yaml_edit(m_active_doc, row_data, new_text_str);
}

// irreducible: 3 params required — document pointer per extraction constraint + row context + new value
void main_window_t::commit_dict_edit(
    dict_document_t * dict_doc,
    const table_row_t * row_data,
    const std::string & new_text_str)
{
	const auto result = m_editor_controller.commit(*dict_doc, *row_data, new_text_str);
	if (!result.success)
		return;

	if (result.propagated_count > 0)
	{
		statusBar()->showMessage(QString("Propagated to %1 entries").arg(result.propagated_count), 5000);
		m_table_model->update_row(m_editor_controller.current_row(), result.new_text, result.status);
		sync_propagated_rows(dict_doc);
		set_unsaved_changes(dict_doc->is_dirty());
		m_editor_controller.set_loaded_text(m_editor_view->translation_editor()->toPlainText());
		update_status_counts();
		return;
	}

	m_table_model->update_row(m_editor_controller.current_row(), result.new_text, result.status);
	set_unsaved_changes(dict_doc->is_dirty());
	m_editor_controller.set_loaded_text(m_editor_view->translation_editor()->toPlainText());
	update_status_counts();
}

// irreducible: 3 params required — document pointer per extraction constraint + row context + new value
void main_window_t::commit_yaml_edit(
    document_t * target_doc,
    const table_row_t * row_data,
    const std::string & new_text_str)
{
	target_doc->commit_edit(row_data->type, row_data->record_index, new_text_str);
	m_table_model->update_row(m_editor_controller.current_row(), new_text_str, status_t::in_progress);

	auto * yaml_doc = dynamic_cast<yaml_document_t *>(target_doc);
	if (yaml_doc)
		yaml_doc->save_tmp();

	set_unsaved_changes(target_doc->is_dirty());
	m_editor_controller.set_loaded_text(m_editor_view->translation_editor()->toPlainText());
	update_status_counts();
}

void main_window_t::sync_propagated_rows(dict_document_t * dict_doc)
{
	auto & data = dict_doc->data_mut();

	for (int i = 0; i < m_table_model->rowCount(); ++i)
	{
		if (i == m_editor_controller.current_row())
			continue;

		const auto * row = m_table_model->row_at(i);
		if (!row)
			continue;

		auto chap_it = data.find(row->type);
		if (chap_it == data.end())
			continue;

		if (row->record_index >= chap_it->second.records.size())
			continue;

		const auto & record = chap_it->second.records[row->record_index];
		if (record.new_text != row->new_text || record.status != row->status)
			m_table_model->update_row(i, record.new_text, record.status);
	}
}

// irreducible: sequential orchestrator — each step depends on prior state; no nesting to flatten
void main_window_t::load_record(int row)
{
	m_editor_controller.set_loading(true);

	if (!m_table_model || !m_editor_view)
	{
		m_editor_controller.set_current_row(-1);
		m_editor_controller.set_loading(false);
		return;
	}

	const auto * row_data = m_table_model->row_at(row);
	if (!row_data)
	{
		load_record_clear(row);
		return;
	}

	const auto load_result =
	    m_active_doc ? m_editor_controller.load(*m_active_doc, *row_data) : editor_load_result_t {};

	if (row_data->type == tools_t::rec_type_t::sctx || row_data->type == tools_t::rec_type_t::bnam)
		load_record_script(row_data);
	else
		load_record_plain(row_data);

	m_editor_view->translation_editor()->setReadOnly(load_result.is_read_only);

	m_hl_original->set_record_type(row_data->type);
	m_hl_adapted->set_record_type(row_data->type);
	m_hl_translation->set_record_type(row_data->type);

	const auto validation_result = m_byte_limit_validator.validate(row_data->type, row_data->new_text);
	m_validation_view->update_validation(validation_result);

	if (row_data->type == tools_t::rec_type_t::text)
		m_book_preview_view->set_html(row_data->old_text, row_data->new_text);
	else
		m_book_preview_view->clear();

	m_annotations_view->update_annotations(
	    load_result.annotations, load_result.speaker_name, load_result.gender, load_result.enchantment);

	m_translation_tab->set_source_text(row_data->old_text);

	if (!load_result.details.empty())
		m_editor_view->set_details(load_result.details);
	else
		m_editor_view->clear_details();

	const auto & annotations = load_result.annotations;
	const auto original_text_lower = m_editor_view->original_view()->toPlainText().toLower();
	const auto translation_text_lower = m_editor_view->translation_editor()->toPlainText().toLower();

	const highlight_config_t orig_config { &annotations, true, highlight_sort_policy_t::length_first };
	auto orig_highlights = find_annotation_highlights(original_text_lower, orig_config);

	m_extra_sel_original.annotations = build_highlight_selections(m_editor_view->original_view(), orig_highlights);
	m_extra_sel_original.grammar.clear();
	m_extra_sel_original.adapted_diff.clear();
	apply_extra_selections(m_editor_view->original_view(), m_extra_sel_original);

	const highlight_config_t trans_config { &annotations, false, highlight_sort_policy_t::length_first };
	auto trans_highlights = find_annotation_highlights(translation_text_lower, trans_config);

	m_extra_sel_translation.annotations =
	    build_highlight_selections(m_editor_view->translation_editor(), trans_highlights);
	m_extra_sel_translation.grammar = m_grammar_check->isChecked()
	                                      ? m_grammar_checker.check(m_editor_view->translation_editor(), row_data->type)
	                                      : QList<QTextEdit::ExtraSelection> {};
	m_extra_sel_translation.adapted_diff.clear();
	apply_extra_selections(m_editor_view->translation_editor(), m_extra_sel_translation);

	m_extra_sel_adapted.annotations.clear();
	m_extra_sel_adapted.grammar.clear();
	m_extra_sel_adapted.adapted_diff.clear();

	if (row_data->status == status_t::adapted && !load_result.details.empty())
	{
		m_extra_sel_adapted.adapted_diff =
		    m_editor_view->highlight_adapted_diff(row_data->new_text, load_result.details, false);
	}
	else if (row_data->status == status_t::changed && !load_result.details.empty())
	{
		m_extra_sel_adapted.adapted_diff =
		    m_editor_view->highlight_adapted_diff(row_data->old_text, load_result.details, true);
	}

	apply_extra_selections(m_editor_view->details_view(), m_extra_sel_adapted);

	const auto history = m_edit_history.get_history(row_data->type, row_data->key_text);
	m_history_view->update_history(history, !load_result.is_read_only);

	m_editor_controller.set_loaded_text(m_editor_view->translation_editor()->toPlainText());
	m_editor_controller.set_current_row(row);

	auto cursor = m_editor_view->translation_editor()->textCursor();
	cursor.movePosition(QTextCursor::End);
	m_editor_view->translation_editor()->setTextCursor(cursor);

	m_editor_controller.set_loading(false);
}

void main_window_t::load_record_clear(int /*row*/)
{
	m_editor_view->original_view()->clear();
	m_editor_view->translation_editor()->clear();
	m_validation_view->clear();
	m_annotations_view->clear();
	m_history_view->clear();
	m_book_preview_view->clear();
	m_editor_controller.set_current_row(-1);
	m_editor_controller.set_loading(false);
}

void main_window_t::load_record_script(const table_row_t * row_data)
{
	m_editor_view->load_script_entry(row_data->old_text, row_data->new_text);
	m_editor_view->translation_editor()->set_block_multiline(true);

	if (row_data->status != status_t::untranslated)
		return;

	int line_count = m_editor_view->script_slot_count();
	QString empty_lines;
	for (int i = 1; i < static_cast<int>(line_count); ++i)
		empty_lines += '\n';

	m_editor_view->translation_editor()->setPlainText(empty_lines);
}

void main_window_t::load_record_plain(const table_row_t * row_data)
{
	m_editor_view->original_view()->setPlainText(QString::fromStdString(row_data->old_text));
	m_editor_view->clear_script_template();

	if (row_data->status == status_t::untranslated)
		m_editor_view->translation_editor()->setPlainText(QString());
	else
		m_editor_view->translation_editor()->setPlainText(QString::fromStdString(row_data->new_text));

	bool block_multiline =
	    (row_data->type == tools_t::rec_type_t::cell || row_data->type == tools_t::rec_type_t::dial ||
	     row_data->type == tools_t::rec_type_t::fnam);
	m_editor_view->translation_editor()->set_block_multiline(block_multiline);
}

// irreducible: sort-then-overlap algorithm with two policy branches; further splitting obscures the logic
std::vector<annotation_highlight_t> main_window_t::find_annotation_highlights(
    const QString & text_lower,
    const highlight_config_t & config)
{
	struct candidate_t
	{
		int start;
		int length;
		bool is_hyperlink;
	};

	std::vector<candidate_t> candidates;

	for (const auto & ann : *config.annotations)
	{
		const auto & raw = config.use_old_text ? ann.old_text : ann.new_text;
		if (raw.empty())
			continue;

		bool is_hl = (ann.kind == annotation_t::dial_topic);
		const auto term = QString::fromStdString(raw).toLower();
		int pos = 0;
		while ((pos = text_lower.indexOf(term, pos)) != -1)
		{
			candidates.push_back({ pos, static_cast<int>(term.length()), is_hl });
			pos += static_cast<int>(term.length());
		}
	}

	if (config.sort_policy == highlight_sort_policy_t::hyperlink_first)
	{
		std::sort(
		    candidates.begin(),
		    candidates.end(),
		    [](const candidate_t & a, const candidate_t & b)
		{
			if (a.is_hyperlink != b.is_hyperlink)
				return a.is_hyperlink;

			if (a.length != b.length)
				return a.length > b.length;

			return a.start < b.start;
		});
	}
	else
	{
		std::sort(
		    candidates.begin(),
		    candidates.end(),
		    [](const candidate_t & a, const candidate_t & b)
		{
			if (a.length != b.length)
				return a.length > b.length;

			if (a.is_hyperlink != b.is_hyperlink)
				return a.is_hyperlink;

			return a.start < b.start;
		});
	}

	std::vector<bool> covered(text_lower.length(), false);
	std::vector<annotation_highlight_t> results;

	for (const auto & c : candidates)
	{
		bool overlap = false;
		for (int i = c.start; i < c.start + c.length; ++i)
		{
			if (covered[i])
			{
				overlap = true;
				break;
			}
		}

		if (overlap)
			continue;

		for (int i = c.start; i < c.start + c.length; ++i)
			covered[i] = true;

		results.push_back({ c.start, c.length, c.is_hyperlink });
	}

	return results;
}

QList<QTextEdit::ExtraSelection> main_window_t::build_highlight_selections(
    translation_edit_view_t * target_editor,
    const std::vector<annotation_highlight_t> & highlights)
{
	QList<QTextEdit::ExtraSelection> selections;

	for (const auto & highlight : highlights)
	{
		QTextEdit::ExtraSelection sel;
		sel.format.setBackground(highlight.is_hyperlink ? QColor(200, 220, 255) : QColor(200, 240, 200));
		sel.cursor = QTextCursor(target_editor->document());
		sel.cursor.setPosition(highlight.start);
		sel.cursor.setPosition(highlight.start + highlight.length, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

	return selections;
}

void main_window_t::on_whitespace_toggled(bool checked)
{
	if (!m_editor_view)
		return;

	QTextOption opt;
	if (checked)
		opt.setFlags(QTextOption::ShowTabsAndSpaces);

	m_editor_view->original_view()->document()->setDefaultTextOption(opt);
	m_editor_view->translation_editor()->document()->setDefaultTextOption(opt);
}

void main_window_t::on_encoding_changed(int index)
{
	constexpr codepage_t codepages[] = {
		codepage_t::windows_1250,
		codepage_t::windows_1251,
		codepage_t::windows_1252,
	};

	if (index < 0 || index >= static_cast<int>(std::size(codepages)))
		return;

	const auto new_codepage = codepages[index];
	if (new_codepage == m_current_codepage)
		return;

	m_current_codepage = new_codepage;
	m_session.set_codepage(new_codepage);
	m_byte_limit_validator.set_codepage(new_codepage);
	m_settings.set_encoding_index(index);
	save_config();

	statusBar()->showMessage(
	    "Encoding changed. Open documents keep their original encoding until "
	    "re-opened.",
	    5000);
}

void main_window_t::on_open_settings()
{
	const auto dict_dir = QCoreApplication::applicationDirPath().toStdString() + "/dictionaries";
	translator_settings_dialog_t dialog(m_settings, dict_dir, this);

	connect(
	    &dialog,
	    &translator_settings_dialog_t::settings_applied,
	    this,
	    [this](const std::string & category) { on_settings_applied(category); });

	dialog.exec();
}

void main_window_t::on_settings_applied(const std::string & category)
{
	if (category == "all" || category == "language")
	{
		on_encoding_changed(m_settings.encoding_index());
		on_spell_lang_changed(m_settings.spell_lang_index());
		m_translation_tab->apply_provider_settings(m_settings);
	}

	if (category == "all" || category == "translation")
		m_translation_tab->apply_provider_settings(m_settings);

	if (category == "all" || category == "shortcuts")
		register_shortcuts();

	save_config();
}

void main_window_t::register_shortcuts()
{
	if (!m_copy_original_action)
	{
		m_copy_original_action = new QAction(this);
		m_copy_original_action->setToolTip("Copy original text to translation (F8)");
		m_copy_original_action->setShortcutContext(Qt::WindowShortcut);
		addAction(m_copy_original_action);
		connect(m_copy_original_action, &QAction::triggered, this, [this]() { shortcut_copy_original(); });
	}

	if (!m_set_in_progress_action)
	{
		m_set_in_progress_action = new QAction(this);
		m_set_in_progress_action->setToolTip("Set status to In Progress (F9)");
		m_set_in_progress_action->setShortcutContext(Qt::WindowShortcut);
		addAction(m_set_in_progress_action);
		connect(
		    m_set_in_progress_action,
		    &QAction::triggered,
		    this,
		    [this]() { shortcut_commit_status(status_t::in_progress); });
	}

	if (!m_set_translated_action)
	{
		m_set_translated_action = new QAction(this);
		m_set_translated_action->setToolTip("Set status to Translated (F10)");
		m_set_translated_action->setShortcutContext(Qt::WindowShortcut);
		addAction(m_set_translated_action);
		connect(
		    m_set_translated_action,
		    &QAction::triggered,
		    this,
		    [this]() { shortcut_commit_status(status_t::translated); });
	}

	const auto resolve = [this](const std::string & action_name, const std::string & fallback)
	{
		const auto stored = m_settings.shortcut(action_name);
		return QKeySequence(QString::fromStdString(stored.empty() ? fallback : stored));
	};

	m_copy_original_action->setShortcut(resolve("copy_original", "F8"));
	m_set_in_progress_action->setShortcut(resolve("set_in_progress", "F9"));
	m_set_translated_action->setShortcut(resolve("set_translated", "F10"));

	if (m_save_action)
		m_save_action->setShortcut(resolve("save", "Ctrl+S"));

	if (m_find_action)
		m_find_action->setShortcut(resolve("find", "Ctrl+F"));

	if (m_settings_action)
		m_settings_action->setShortcut(resolve("settings", "Ctrl+,"));

	if (m_escape_action)
		m_escape_action->setShortcut(resolve("escape", "Escape"));
}

void main_window_t::shortcut_copy_original()
{
	if (m_editor_controller.current_row() < 0)
		return;

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (!dict_doc)
		return;

	const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
	if (!row_data)
		return;

	m_editor_controller.copy_original(*dict_doc, *row_data);
	m_table_model->update_row(m_editor_controller.current_row(), row_data->old_text, status_t::in_progress);
	set_unsaved_changes(true);
	update_status_counts();
	load_record(m_editor_controller.current_row());
}

void main_window_t::shortcut_commit_status(status_t new_status)
{
	if (m_editor_controller.current_row() < 0)
		return;

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (!dict_doc)
		return;

	const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
	if (!row_data)
		return;

	const auto result = m_editor_controller.commit_status(*dict_doc, *row_data, new_status);
	if (!result.success)
		return;

	m_table_model->update_row(m_editor_controller.current_row(), result.new_text, result.status);
	set_unsaved_changes(true);
	update_status_counts();
}

void main_window_t::rebuild_annotations()
{
	std::vector<dict_source_t> sources;
	for (auto * dict_doc : m_session.all_dicts())
		sources.push_back({ &dict_doc->data(), dict_doc->path() });

	m_glossary.rebuild(sources);
}

void main_window_t::save_current_filter_state()
{
	if (!m_active_doc)
		return;

	filter_state_t state;
	state.type_filter = m_type_filter;
	state.sub_type_filter = m_filter_tree_view->get_active_sub_types();
	state.status_filter = m_status_filter;
	state.type_filter_solo = m_type_filter_solo;
	m_filter_states[m_active_doc->path()] = std::move(state);
}

void main_window_t::restore_filter_state(const std::string & path)
{
	auto it = m_filter_states.find(path);
	if (it != m_filter_states.end())
	{
		m_type_filter = it->second.type_filter;
		m_status_filter = it->second.status_filter;
		m_type_filter_solo = it->second.type_filter_solo;
		m_filter_tree_view->set_active_types(it->second.type_filter);
		m_filter_tree_view->set_active_sub_types(it->second.sub_type_filter);
	}
	else
	{
		m_type_filter = {
			tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
			tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
			tools_t::rec_type_t::indx, tools_t::rec_type_t::sctx,
		};
		m_status_filter.clear();
		m_type_filter_solo = false;
		m_filter_tree_view->set_active_types(m_type_filter);
		m_filter_tree_view->set_active_sub_types({});
	}
}

void main_window_t::rebuild_sidebar()
{
	std::string active_path;
	if (m_active_doc)
		active_path = m_active_doc->path();

	auto model = build_render_model(m_file_list, m_session, active_path);
	m_sidebar->set_model(model);
}

void main_window_t::update_sidebar_item(const std::string & path)
{
	const auto * fe = m_file_list.get(path);
	if (!fe)
		return;

	const auto * doc = m_session.find(path);
	const bool is_loaded = (doc != nullptr);
	const bool is_dirty = doc && doc->is_dirty();
	m_sidebar->update_item_text(fe->path, derive_display_name(*fe, is_loaded, is_dirty));
}

void main_window_t::update_annotations()
{
	if (m_editor_controller.current_row() < 0)
		return;

	const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
	if (!row_data)
		return;

	const auto annotations = m_glossary.annotate(row_data->old_text, row_data->type);

	std::string speaker_name;
	std::string gender_str;
	std::string enchantment_str;

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (dict_doc)
	{
		const auto & data = dict_doc->data();
		auto it = data.find(row_data->type);
		if (it != data.end() && row_data->record_index < it->second.records.size())
		{
			const auto & entry = it->second.records[row_data->record_index];
			speaker_name = entry.speaker_name;
			gender_str = entry.gender;
			enchantment_str = entry.enchantment;
		}
	}

	m_annotations_view->update_annotations(annotations, speaker_name, gender_str, enchantment_str);
}

void main_window_t::apply_extra_selections(translation_edit_view_t * editor, const extra_selections_state_t & state)
{
	QList<QTextEdit::ExtraSelection> merged;
	merged.append(state.annotations);
	merged.append(state.grammar);
	merged.append(state.adapted_diff);
	editor->setExtraSelections(merged);
}

void main_window_t::update_status_counts()
{
	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (!dict_doc)
		return;

	const table_filter_params_t filter_params {
		m_type_filter, m_filter_tree_view->get_active_sub_types(), m_status_filter, m_row_filter, m_type_filter_solo
	};

	auto result = build_filtered_rows(dict_doc->data(), filter_params);

	size_t total = 0;
	size_t total_translated = 0;
	for (const auto & [t, c] : result.counts.type_counts)
		total += c;
	for (const auto & [t, c] : result.counts.translated_counts)
		total_translated += c;

	m_filter_tree_view->update_counts(result.counts.type_counts, result.counts.translated_counts);
	m_filter_tree_view->update_sub_type_counts(
	    result.counts.sub_type_total_counts, result.counts.sub_type_translated_counts);
	m_filter_tree_view->set_total_count(total_translated, total);
	m_status_filter_view->update_counts(result.counts.filtered_status_counts, result.counts.total_status_counts);
}

void main_window_t::update_validation()
{
	if (m_editor_controller.current_row() < 0)
		return;

	const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
	if (!row_data)
		return;

	const auto current_text = m_editor_view->translation_editor()->toPlainText().toStdString();
	const auto result = m_byte_limit_validator.validate(row_data->type, current_text);
	m_validation_view->update_validation(result);
}

void main_window_t::scan_spell_dictionaries()
{}

void main_window_t::on_spell_lang_changed(int index)
{
	if (index <= 0)
	{
		m_hl_translation->set_spell_checker(nullptr);
		return;
	}

	const auto aff_path = m_settings.spell_aff_path();
	const auto dic_path = m_settings.spell_dic_path();

	if (aff_path.empty() || dic_path.empty())
	{
		m_hl_translation->set_spell_checker(nullptr);
		return;
	}

	if (!m_spell_checker.load(aff_path, dic_path))
		return;

	m_hl_translation->set_spell_checker(&m_spell_checker);
}

void main_window_t::load_config()
{
	move(m_settings.window_x(), m_settings.window_y());
	resize(m_settings.window_width(), m_settings.window_height());

	if (m_settings.window_maximized())
		showMaximized();

	const int encoding_index = m_settings.encoding_index();
	constexpr codepage_t codepages_table[] = {
		codepage_t::windows_1250,
		codepage_t::windows_1251,
		codepage_t::windows_1252,
	};
	if (encoding_index >= 0 && encoding_index < 3)
		m_current_codepage = codepages_table[encoding_index];

	m_session.set_codepage(m_current_codepage);

	m_translation_tab->apply_provider_settings(m_settings);

	m_sidebar_toggle->setChecked(m_settings.sidebar_visible());
	m_bottom_panel_toggle->setChecked(m_settings.bottom_visible());

	const float split_ratio = m_settings.split_ratio();
	if (split_ratio > 0.0f)
		m_editor_view->set_split_ratio(split_ratio);

	std::vector<int> col_widths;
	for (int i = 0; i < 4; ++i)
		col_widths.push_back(m_settings.column_width(i));
	m_table_view->set_column_widths(col_widths);

	const auto workspace = (QCoreApplication::applicationDirPath() + "/workspace").toStdString();
	std::vector<std::string> roots = { workspace };
	for (const auto & r : m_settings.workspace_roots())
	{
		if (r != workspace)
			roots.push_back(r);
	}
	m_file_list.scan_roots(roots);
	scan_workspace();

	const auto active_path = m_settings.active_dict_path();
	if (!active_path.empty())
	{
		auto * doc = m_session.open(active_path);
		if (doc)
			switch_document(doc);
	}

	rebuild_sidebar();
	rebuild_table();

	const int spell_index = m_settings.spell_lang_index();
	on_spell_lang_changed(spell_index);

	update_watcher_paths();
	register_shortcuts();
}

void main_window_t::save_config()
{
	m_settings.set_window_x(pos().x());
	m_settings.set_window_y(pos().y());
	m_settings.set_window_width(size().width());
	m_settings.set_window_height(size().height());
	m_settings.set_window_maximized(isMaximized());

	m_settings.set_sidebar_visible(m_sidebar_toggle->isChecked());
	m_settings.set_bottom_visible(m_bottom_panel_toggle->isChecked());

	m_settings.set_split_ratio(static_cast<float>(m_editor_view->get_split_ratio()));

	const auto col_widths = m_table_view->get_column_widths();
	for (int i = 0; i < static_cast<int>(col_widths.size()) && i < 4; ++i)
		m_settings.set_column_width(i, col_widths[i]);

	m_settings.set_active_dict_path(m_active_doc ? m_active_doc->path() : std::string {});
	m_settings.set_workspace_roots(m_file_list.get_roots());

	m_settings.sync();
}

// irreducible: switch dispatch for 5 operation types; each branch contains distinct dialog/executor calls
void main_window_t::on_plugin_operation(const std::string & plugin_path_arg, plugin_op_t op)
{
	const auto plugin_path = plugin_path_arg;
	const auto encoding = m_current_codepage;

	auto path_sep = plugin_path.find_last_of("/\\");
	auto plugin_dir = path_sep != std::string::npos ? plugin_path.substr(0, path_sep) : std::string {};
	plugin_dir = string_utils::normalize_path(plugin_dir);

	const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";

	if (op == plugin_op_t::convert || op == plugin_op_t::create_plugin)
		m_executor.set_output_dir(plugin_dir);
	else
		m_executor.set_output_dir(workspace_dir);

	if (m_session.has_any_unsaved())
	{
		auto answer = QMessageBox::question(
		    this,
		    "Unsaved Changes",
		    "Some dictionaries have unsaved changes. Save before proceeding?",
		    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

		if (answer == QMessageBox::Cancel)
			return;

		if (answer == QMessageBox::Yes)
			m_session.save_all();
	}

	operation_executor_t::result_t result;

	switch (op)
	{
	case plugin_op_t::make_dict:
	{
		result = m_executor.make_dict(plugin_path, encoding);
		break;
	}
	case plugin_op_t::make_dict_with_base:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, m_settings.last_merge_order(), this);
		dialog.setWindowTitle("Select Dictionaries");
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		m_settings.set_last_merge_order(selected);

		for (const auto & sel_path : selected)
			m_session.open(sel_path);

		dict_merger_t merger(selected);
		result = m_executor.make_dict_with_base(plugin_path, merger.get_dict(), encoding);
		break;
	}
	case plugin_op_t::make_base:
	{
		auto params = show_make_base_dialog(plugin_path);
		if (!params.has_value())
			return;

		result = m_executor.make_base(
		    plugin_path,
		    params->native_path,
		    params->foreign_lang,
		    params->native_lang,
		    nullptr,
		    params->base_mode,
		    params->dictionary_aff_path);
		break;
	}
	case plugin_op_t::convert:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, m_settings.last_merge_order(), this);
		dialog.setWindowTitle("Select Dictionaries for Convert");
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		m_settings.set_last_merge_order(selected);

		for (const auto & sel_path : selected)
			m_session.open(sel_path);

		result = m_executor.convert(plugin_path, selected, encoding);
		break;
	}
	case plugin_op_t::create_plugin:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, m_settings.last_merge_order(), this);
		dialog.setWindowTitle("Select Dictionaries for Create");
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		m_settings.set_last_merge_order(selected);

		for (const auto & sel_path : selected)
			m_session.open(sel_path);

		result = m_executor.create_plugin(plugin_path, selected, encoding);
		break;
	}
	}

	log_operation_result(plugin_path, op, result);

	if (!result.output_path.empty())
	{
		auto norm_output = string_utils::normalize_path(result.output_path);

		if (m_active_doc && m_active_doc->path() == norm_output)
			m_active_doc = nullptr;

		m_session.close(result.output_path);
	}

	scan_workspace();
}

// irreducible: self-contained modal dialog — UI construction is inherently verbose
std::optional<make_base_params_t> main_window_t::show_make_base_dialog(const std::string & plugin_path)
{
	auto source_sep = plugin_path.find_last_of("/\\");
	auto source_filename = source_sep != std::string::npos ? plugin_path.substr(source_sep + 1) : plugin_path;
	auto source_lower = source_filename;
	std::transform(
	    source_lower.begin(),
	    source_lower.end(),
	    source_lower.begin(),
	    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	struct plugin_item_t
	{
		std::string path;
		std::string display;
		std::string filename;
		std::string root_path;
		std::string subfolder;
	};

	std::vector<plugin_item_t> plugins;

	for (const auto * entry : m_file_list.all())
	{
		if (entry->type != file_type_t::plugin)
			continue;

		if (entry->path == plugin_path)
			continue;

		plugins.push_back(
		    { entry->path,
		      derive_display_name(*entry, false, false),
		      entry->filename,
		      entry->root_path,
		      entry->workspace_subfolder });
	}

	if (plugins.empty())
		return std::nullopt;

	std::sort(
	    plugins.begin(),
	    plugins.end(),
	    [](const plugin_item_t & a, const plugin_item_t & b) { return a.filename < b.filename; });

	QDialog dlg(this);
	dlg.setWindowTitle("Make Base");
	dlg.setModal(true);
	dlg.resize(400, 350);

	auto * dlg_layout = new QVBoxLayout(&dlg);
	dlg_layout->addWidget(new QLabel("Select the native ESM:", &dlg));

	auto * tree = new QTreeWidget(&dlg);
	tree->setHeaderHidden(true);
	tree->setRootIsDecorated(true);
	tree->setIndentation(16);
	dlg_layout->addWidget(tree);

	struct root_builder_t
	{
		std::map<std::string, std::vector<plugin_item_t *>> subfolder_items;
		std::vector<plugin_item_t *> root_items;
	};

	std::map<std::string, root_builder_t> roots;
	for (auto & p : plugins)
	{
		auto & rb = roots[p.root_path];
		if (p.subfolder.empty())
			rb.root_items.push_back(&p);
		else
			rb.subfolder_items[p.subfolder].push_back(&p);
	}

	QTreeWidgetItem * best_match_item = nullptr;

	for (auto & [root_path, rb] : roots)
	{
		auto root_label = root_path;
		auto sep = root_label.find_last_of("/\\");
		if (sep != std::string::npos)
			root_label = root_label.substr(sep + 1);

		if (root_label == "workspace")
			root_label = workspace_label;

		auto * root_node = new QTreeWidgetItem(tree);
		root_node->setText(0, QString::fromStdString(root_label));
		root_node->setFlags(Qt::ItemIsEnabled);
		QFont bold = root_node->font(0);
		bold.setBold(true);
		root_node->setFont(0, bold);

		auto add_plugin_items = [&](QTreeWidgetItem * parent, std::vector<plugin_item_t *> & items)
		{
			for (auto * p : items)
			{
				auto * item = new QTreeWidgetItem(parent);
				item->setText(0, QString::fromStdString(p->display));
				item->setData(0, Qt::UserRole, QString::fromStdString(p->path));
				item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				item->setForeground(0, QColor(100, 180, 100));

				auto name_lower = p->filename;
				std::transform(
				    name_lower.begin(),
				    name_lower.end(),
				    name_lower.begin(),
				    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (name_lower == source_lower && !best_match_item)
					best_match_item = item;
			}
		};

		add_plugin_items(root_node, rb.root_items);

		for (auto & [subfolder, items] : rb.subfolder_items)
		{
			auto * folder_node = new QTreeWidgetItem(root_node);
			folder_node->setText(0, QString::fromStdString(subfolder));
			folder_node->setFlags(Qt::ItemIsEnabled);
			folder_node->setForeground(0, QColor(130, 130, 130));

			add_plugin_items(folder_node, items);
			folder_node->setExpanded(true);
		}

		root_node->setExpanded(true);
	}

	if (best_match_item)
		tree->setCurrentItem(best_match_item);

	auto * mode_group = new QGroupBox("Identical text handling", &dlg);
	auto * mode_layout = new QVBoxLayout(mode_group);
	auto * radio_full = new QRadioButton("Full (identical marked as Translated)", mode_group);
	auto * radio_partial = new QRadioButton("Partial (identical marked as Untranslated)", mode_group);
	radio_full->setChecked(true);
	radio_full->setToolTip("Use to create base dictionary from a fully translated file");
	radio_partial->setToolTip("Use to create base dictionary from a partially translated file");
	mode_layout->addWidget(radio_full);
	mode_layout->addWidget(radio_partial);

	auto * partial_explanation = new QLabel(mode_group);
	partial_explanation->setWordWrap(true);
	partial_explanation->setText(
	    "When old_text equals new_text, the text is tokenized by non-alphanumeric "
	    "characters (tokens shorter than 3 characters are ignored). Each token is "
	    "checked against the Hunspell dictionary:\n"
	    "\u2022 If any token is found \u2192 status = Untranslated (contains source language words)\n"
	    "\u2022 If no tokens are found \u2192 status = To Verify (likely a proper noun)");
	partial_explanation->setStyleSheet("color: #888; margin-left: 20px; margin-top: 4px;");
	partial_explanation->setVisible(false);
	mode_layout->addWidget(partial_explanation);

	auto * dict_label = new QLabel("Source dictionary:", mode_group);
	dict_label->setStyleSheet("margin-left: 20px; margin-top: 4px;");
	dict_label->setVisible(false);
	mode_layout->addWidget(dict_label);

	auto * dict_combo = new QComboBox(mode_group);
	dict_combo->setToolTip("Hunspell dictionary used to detect untranslated words");
	dict_combo->setVisible(false);
	mode_layout->addWidget(dict_combo);

	{
		auto dict_dir = tools_t::get_exe_dir();
		if (!dict_dir.empty() && dict_dir.back() != '/' && dict_dir.back() != '\\')
			dict_dir += '/';
		dict_dir += "dictionaries";

		const std::filesystem::path dict_path(dict_dir);
		if (std::filesystem::exists(dict_path))
		{
			for (const auto & entry : std::filesystem::directory_iterator(dict_path))
			{
				if (!entry.is_regular_file())
					continue;

				if (entry.path().extension().string() != ".aff")
					continue;

				auto dic_file = entry.path();
				dic_file.replace_extension(".dic");
				if (!std::filesystem::exists(dic_file))
					continue;

				const auto stem = entry.path().stem().string();
				const auto aff_path = entry.path().string();
				dict_combo->addItem(QString::fromStdString(stem), QString::fromStdString(aff_path));
			}
		}
	}

	const auto partial_aff_setting = m_settings.partial_dict_aff_path();
	if (!partial_aff_setting.empty())
	{
		for (int i = 0; i < dict_combo->count(); ++i)
		{
			if (dict_combo->itemData(i).toString().toStdString() == partial_aff_setting)
			{
				dict_combo->setCurrentIndex(i);
				break;
			}
		}
	}

	connect(radio_partial, &QRadioButton::toggled, partial_explanation, &QLabel::setVisible);
	connect(radio_partial, &QRadioButton::toggled, dict_label, &QLabel::setVisible);
	connect(radio_partial, &QRadioButton::toggled, dict_combo, &QComboBox::setVisible);

	dlg_layout->addWidget(mode_group);

	auto * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
	dlg_layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
	connect(
	    tree,
	    &QTreeWidget::itemDoubleClicked,
	    &dlg,
	    [&dlg](QTreeWidgetItem * item, int)
	{
		if (item && (item->flags() & Qt::ItemIsSelectable))
			dlg.accept();
	});

	if (dlg.exec() != QDialog::Accepted)
		return std::nullopt;

	auto * selected_item = tree->currentItem();
	if (!selected_item || !(selected_item->flags() & Qt::ItemIsSelectable))
		return std::nullopt;

	const auto native_path = selected_item->data(0, Qt::UserRole).toString().toStdString();
	const auto * native_entry = m_file_list.get(native_path);

	std::string foreign_lang;
	const auto * foreign_entry = m_file_list.get(plugin_path);
	if (foreign_entry)
		foreign_lang = foreign_entry->language_tag;

	std::string native_lang;
	if (native_entry)
		native_lang = native_entry->language_tag;

	make_base_params_t params;
	params.native_path = native_path;
	params.foreign_lang = foreign_lang;
	params.native_lang = native_lang;
	params.base_mode = radio_partial->isChecked() ? base_mode_t::partial : base_mode_t::full;
	if (radio_partial->isChecked() && dict_combo->count() > 0)
		params.dictionary_aff_path = dict_combo->currentData().toString().toStdString();
	return params;
}

// irreducible: 3 params — source path + operation type + result data
void main_window_t::log_operation_result(
    const std::string & plugin_path,
    plugin_op_t op_type,
    const operation_executor_t::result_t & result)
{
	auto sep = plugin_path.find_last_of("/\\");
	auto plugin_name = sep != std::string::npos ? plugin_path.substr(sep + 1) : plugin_path;

	std::string op_name;
	switch (op_type)
	{
	case plugin_op_t::make_dict:
		op_name = "make dict: " + plugin_name;
		break;
	case plugin_op_t::make_dict_with_base:
		op_name = "make dict with base: " + plugin_name;
		break;
	case plugin_op_t::make_base:
		op_name = "make base: " + plugin_name;
		break;
	case plugin_op_t::convert:
		op_name = "convert: " + plugin_name;
		break;
	case plugin_op_t::create_plugin:
		op_name = "create: " + plugin_name;
		break;
	}

	m_log_view->append_log(op_name, result.log_text);
	m_record_tabs->setCurrentWidget(m_log_view);
}

// Indivisible batch pattern: shared_ptr + QTimer + lambda
void main_window_t::start_batch_translation(dict_document_t * dict_doc)
{
	m_translation_tab->set_batch_in_progress(true);
	m_translation_tab->append_log("[info] collecting untranslated entries...\n");

	struct batch_state_t
	{
		std::vector<std::pair<tools_t::rec_type_t, size_t>> work_items;
		size_t current = 0;
		int translated_count = 0;
		int glossary_count = 0;
		int error_count = 0;
	};

	auto state = std::make_shared<batch_state_t>();
	auto * provider = m_translation_tab->ct2_provider();

	auto & data = dict_doc->data_mut();
	for (auto & [type, chapter] : data)
	{
		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			if (chapter.records[i].status == status_t::untranslated && !chapter.records[i].old_text.empty())
				state->work_items.push_back({ type, i });

			if (state->work_items.size() >= 10)
				break;
		}

		if (state->work_items.size() >= 10)
			break;
	}

	if (state->work_items.empty())
	{
		m_translation_tab->append_log("[info] no untranslated entries found\n");
		m_translation_tab->set_batch_in_progress(false);
		return;
	}

	m_translation_tab->append_log("[info] translating " + std::to_string(state->work_items.size()) + " entries\n");

	auto * timer = new QTimer(this);
	timer->setInterval(0);
	connect(
	    timer,
	    &QTimer::timeout,
	    this,
	    [this, state, dict_doc, provider, timer]()
	{
		if (state->current >= state->work_items.size())
		{
			timer->stop();
			timer->deleteLater();

			dict_doc->set_dirty(true);
			set_unsaved_changes(true);
			update_sidebar_item(dict_doc->path());

			if (m_editor_controller.current_row() >= 0)
				load_record(m_editor_controller.current_row());

			m_translation_tab->append_log(
			    "[info] done: translated=" + std::to_string(state->translated_count) + " glossary=" +
			    std::to_string(state->glossary_count) + " errors=" + std::to_string(state->error_count) + "\n");
			m_translation_tab->set_batch_in_progress(false);
			return;
		}

		const auto & [type, idx] = state->work_items[state->current];
		auto & data_ref = dict_doc->data_mut();
		auto type_it = data_ref.find(type);
		if (type_it == data_ref.end() || idx >= type_it->second.records.size())
		{
			++state->current;
			return;
		}

		auto & record = type_it->second.records[idx];
		auto result = provider->translate_sync(record.old_text);

		if (!result.success)
		{
			m_translation_tab->append_log(
			    "[error] \"" + record.old_text.substr(0, 40) + "...\" -> " + result.error + "\n");
			++state->error_count;
			++state->current;
			return;
		}

		auto glossary_applied = m_glossary.apply_glossary(result.text);
		bool had_glossary = (glossary_applied != result.text);

		record.new_text = glossary_applied;
		record.status = status_t::model;
		dict_doc->modified_records_insert(type, idx);
		++state->translated_count;

		for (int row = 0; row < m_table_model->rowCount(); ++row)
		{
			const auto * r = m_table_model->row_at(row);
			if (r && r->type == type && r->record_index == idx)
			{
				m_table_model->update_row(row, record.new_text, record.status);
				break;
			}
		}

		if (had_glossary)
			++state->glossary_count;

		++state->current;
	});
	timer->start();
}

void main_window_t::on_plugin_unload(const std::string & path)
{
	m_file_list.remove(path);
	rebuild_sidebar();
}

void main_window_t::scan_workspace()
{
	const auto workspace = (QCoreApplication::applicationDirPath() + "/workspace").toStdString();
	QDir().mkpath(QString::fromStdString(workspace));

	std::vector<std::string> roots;
	roots.push_back(workspace);
	for (const auto & r : m_file_list.get_roots())
	{
		if (r != workspace)
			roots.push_back(r);
	}

	m_file_list.scan_roots(roots);

	bool any_loaded = false;
	for (const auto * entry : m_file_list.all())
	{
		if (!entry->is_workspace)
			continue;

		if (entry->type != file_type_t::base_dict && entry->type != file_type_t::user_dict)
			continue;

		if (m_session.find(entry->path))
			continue;

		if (m_session.open(entry->path))
			any_loaded = true;
	}

	if (any_loaded)
	{
		rebuild_annotations();
		m_last_annotation_version = m_session.dict_version();
	}

	rebuild_sidebar();
}

void main_window_t::update_watcher_paths()
{
	const auto current = m_fs_watcher->directories();
	if (!current.isEmpty())
		m_fs_watcher->removePaths(current);

	QStringList paths;

	const auto workspace = QCoreApplication::applicationDirPath() + "/workspace";
	add_directory_recursive(paths, workspace);

	for (const auto & root : m_file_list.get_roots())
		add_directory_recursive(paths, QString::fromStdString(root));

	if (!paths.isEmpty())
		m_fs_watcher->addPaths(paths);
}

void main_window_t::add_directory_recursive(QStringList & target_paths, const QString & directory)
{
	QDir qdir(directory);
	if (!qdir.exists())
		return;

	target_paths.append(directory);

	QDirIterator it(directory, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (it.hasNext())
		target_paths.append(it.next());
}

std::vector<dict_selection_dialog_t::dict_entry_t> main_window_t::build_dict_entries(
    const std::string & source_dir) const
{
	std::set<std::string> seen;
	std::vector<dict_selection_dialog_t::dict_entry_t> entries;

	auto normalize = [](std::string_view p) { return string_utils::to_lower(string_utils::normalize_path(p)); };

	auto matches_source_dir = [&](const std::string & norm, const std::string & target)
	{
		if (target.empty())
			return false;

		auto dir_sep = norm.find_last_of('/');
		if (dir_sep == std::string::npos)
			return false;

		return norm.substr(0, dir_sep) == target;
	};

	auto target = source_dir.empty() ? std::string {} : normalize(source_dir);

	std::set<std::string> saved_order_set;
	for (const auto & p : m_settings.last_merge_order())
		saved_order_set.insert(normalize(p));
	bool use_saved = !saved_order_set.empty();

	for (const auto * dict_doc : m_session.all_dicts())
	{
		auto norm = normalize(dict_doc->path());
		if (!seen.insert(norm).second)
			continue;

		bool pre = use_saved ? (saved_order_set.count(norm) > 0) : matches_source_dir(norm, target);

		std::string root_path;
		std::string subfolder;
		const auto * fe = m_file_list.get(dict_doc->path());
		if (fe)
		{
			root_path = fe->root_path;
			subfolder = fe->workspace_subfolder;
		}

		entries.push_back(
		    { std::string(string_utils::extract_filename(dict_doc->path())),
		      dict_doc->path(),
		      dict_doc->kind(),
		      root_path,
		      subfolder,
		      pre });
	}

	for (const auto * fe : m_file_list.all())
	{
		if (fe->type != file_type_t::user_dict && fe->type != file_type_t::base_dict)
			continue;

		auto norm = normalize(fe->path);
		if (!seen.insert(norm).second)
			continue;

		bool pre = use_saved ? (saved_order_set.count(norm) > 0) : matches_source_dir(norm, target);

		auto kind = (fe->type == file_type_t::base_dict) ? dict_kind_t::base : dict_kind_t::user;
		entries.push_back({ fe->filename, fe->path, kind, fe->root_path, fe->workspace_subfolder, pre });
	}

	return entries;
}

void main_window_t::on_item_clicked(const std::string & path)
{
	const auto * entry = m_file_list.get(path);
	if (!entry)
		return;

	auto norm_path = string_utils::normalize_path(path);

	if (m_active_doc && m_active_doc->path() == norm_path)
		return;

	auto * doc = m_session.open(path);
	if (!doc)
		return;

	switch_document(doc);
	rebuild_sidebar();
}

void main_window_t::on_operation_requested(const std::string & path, plugin_op_t op)
{
	const auto * entry = m_file_list.get(path);
	if (!entry)
		return;

	const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";
	const auto output_dir = derive_output_dir(*entry, workspace_dir);
	m_executor.set_output_dir(output_dir);

	on_plugin_operation(path, op);
}

void main_window_t::on_save_requested(const std::string & path)
{
	auto norm_path = string_utils::normalize_path(path);

	if (m_active_doc && m_active_doc->path() == norm_path)
	{
		commit_current_edit();
		m_active_doc->save();
		update_sidebar_item(m_active_doc->path());

		m_log_view->append_log("save", "saved \"" + m_active_doc->path() + "\"\r\n");

		if (!m_session.has_any_unsaved())
			set_unsaved_changes(false);

		return;
	}

	auto * doc = m_session.find(path);
	if (!doc)
		return;

	doc->save();
	update_sidebar_item(doc->path());

	m_log_view->append_log("save", "saved \"" + doc->path() + "\"\r\n");

	if (!m_session.has_any_unsaved())
		set_unsaved_changes(false);
}

void main_window_t::on_unload_requested(const std::string & path)
{
	auto * doc = m_session.find(path);
	if (!doc)
	{
		m_file_list.remove(path);
		rebuild_sidebar();
		return;
	}

	if (doc->is_dirty())
	{
		auto answer = QMessageBox::question(
		    this,
		    "Unsaved Changes",
		    "This dictionary has unsaved changes. Save before unloading?",
		    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

		if (answer == QMessageBox::Cancel)
			return;

		if (answer == QMessageBox::Save)
			doc->save();
	}

	if (m_active_doc && m_active_doc->path() == doc->path())
		switch_document(nullptr);

	m_session.close(path);

	auto norm = string_utils::normalize_path(path);
	m_filter_states.erase(norm);

	rebuild_annotations();
	m_last_annotation_version = m_session.dict_version();
	rebuild_sidebar();
	rebuild_table();
}

void main_window_t::on_delete_requested(const std::string & path)
{
	auto sep = path.find_last_of("/\\");
	auto filename = sep != std::string::npos ? path.substr(sep + 1) : path;

	auto answer = QMessageBox::question(
	    this,
	    "Delete File",
	    QString("Delete \"%1\" from disk?").arg(QString::fromStdString(filename)),
	    QMessageBox::Yes | QMessageBox::No);

	if (answer != QMessageBox::Yes)
		return;

	if (!QFile::remove(QString::fromStdString(path)))
	{
		QMessageBox::warning(this, "Error", QString("Failed to delete \"%1\".").arg(QString::fromStdString(filename)));
		return;
	}

	auto norm_del_path = string_utils::normalize_path(path);
	if (m_active_doc && m_active_doc->path() == norm_del_path)
		switch_document(nullptr);

	m_session.close(path);
	m_filter_states.erase(norm_del_path);
	rebuild_annotations();
	m_last_annotation_version = m_session.dict_version();
	m_file_list.remove(path);
	rebuild_sidebar();
	rebuild_table();
	scan_workspace();
}

void main_window_t::closeEvent(QCloseEvent * event)
{
	if (m_session.has_any_unsaved())
	{
		auto answer = QMessageBox::question(
		    this,
		    "Unsaved Changes",
		    "You have unsaved dictionary changes. What would you like to do?",
		    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

		if (answer == QMessageBox::Cancel)
		{
			event->ignore();
			return;
		}

		if (answer == QMessageBox::Save)
			m_session.save_all();
	}

	commit_current_edit();
	if (auto * yaml_doc = dynamic_cast<yaml_document_t *>(m_active_doc))
		yaml_doc->save_tmp();

	save_config();
	QMainWindow::closeEvent(event);
}
