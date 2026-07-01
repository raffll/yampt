#include "main_window.hpp"
#include "editor/grammar_checker.hpp"
#include "dialog/dict_selection_dialog.hpp"
#include "dialog/find_replace_dialog.hpp"
#include "dialog/first_run_dialog.hpp"
#include "dialog/spell_context_menu.hpp"
#include "highlighter/glossary_highlighter.hpp"
#include "highlighter/editor_highlighter.hpp"
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
#include <QActionGroup>
#include <QCloseEvent>
#include <QCoreApplication>
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
    , session_(current_codepage_)
    , editor_controller_(edit_history_, byte_limit_validator_, glossary_)
{
	setWindowTitle("yTranslator");
	resize(1280, 720);
	setMinimumSize(800, 600);

	type_filter_ = {
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

	const auto config_path = QCoreApplication::applicationDirPath() + "/yTranslator.ini";
	bool first_run = !QFile::exists(config_path);

	load_config();

	if (first_run)
	{
		std::vector<std::string> spell_langs;
		const auto & spell_actions = spelling_group_->actions();
		for (int i = 1; i < spell_actions.size(); ++i)
			spell_langs.push_back(spell_actions.at(i)->text().toStdString());

		first_run_dialog_t dialog(spell_langs, this);
		if (dialog.exec() == QDialog::Accepted)
		{
			const int enc_idx = dialog.selected_encoding_index();
			if (enc_idx >= 0 && enc_idx < encoding_group_->actions().size())
				encoding_group_->actions().at(enc_idx)->trigger();

			const int spell_idx = dialog.selected_spell_lang_index();
			if (spell_idx >= 0 && spell_idx < spell_actions.size())
				spell_actions.at(spell_idx)->trigger();

			save_config();
		}
	}
}

void main_window_t::set_unsaved_changes(bool dirty)
{
	if (has_unsaved_changes_ == dirty)
		return;

	has_unsaved_changes_ = dirty;
	setWindowTitle(has_unsaved_changes_ ? "yTranslator *" : "yTranslator");
}

void main_window_t::on_save()
{
	commit_current_edit();

	if (!active_doc_)
		return;

	if (!active_doc_->is_dirty())
		return;

	active_doc_->save();

	update_sidebar_item(active_doc_->path());

	log_view_->append_log("save", "saved \"" + active_doc_->path() + "\"\r\n");

	if (!session_.has_any_unsaved())
		set_unsaved_changes(false);
}

void main_window_t::on_save_all()
{
	commit_current_edit();

	std::string log_msg;
	for (auto * doc : session_.all_dirty())
		log_msg += "saved \"" + doc->path() + "\"\r\n";

	session_.save_all();

	for (auto * doc : session_.all())
		update_sidebar_item(doc->path());

	if (!log_msg.empty())
		log_view_->append_log("save all", log_msg);

	if (!session_.has_any_unsaved())
		set_unsaved_changes(false);
}

void main_window_t::on_find()
{
	search_field_->setFocus();
	search_field_->selectAll();
}

void main_window_t::on_escape()
{
	if (!search_field_->text().isEmpty())
		search_field_->clear();
}

void main_window_t::on_search_changed(const QString & text)
{
	search_query_ = text;

	row_filter_t::config_t cfg;
	cfg.query = text.toStdString();
	cfg.case_sensitive = case_sensitive_check_ && case_sensitive_check_->isChecked();
	cfg.regex_mode = regex_check_ && regex_check_->isChecked();
	cfg.columns.clear();
	if (search_col_key_->isChecked())
		cfg.columns.insert(search_column_t::key);
	if (search_col_original_->isChecked())
		cfg.columns.insert(search_column_t::original);
	if (search_col_translation_->isChecked())
		cfg.columns.insert(search_column_t::translation);
	row_filter_.set_config(cfg);

	rebuild_table();
}

void main_window_t::on_case_sensitive_changed(int /*state*/)
{
	on_search_changed(search_query_);
}

void main_window_t::on_filters_changed()
{
	type_filter_ = filter_tree_view_->get_active_types();
	type_filter_solo_ = filter_tree_view_->has_sub_type_filter();
	rebuild_table();
}

void main_window_t::on_status_filters_changed()
{
	status_filter_ = status_filter_view_->get_active_statuses();
	rebuild_table();
}

void main_window_t::clear_editor_panels()
{
	editor_view_->original_view()->clear();
	editor_view_->translation_editor()->clear();
	editor_view_->clear_details();
	validation_view_->clear();
	annotations_view_->clear();
	history_view_->clear();
	book_preview_view_->clear();
}

void main_window_t::switch_document(document_t * new_doc)
{
	commit_current_edit();
	save_current_filter_state();

	active_doc_ = new_doc;
	editor_controller_.set_current_row(-1);

	if (!active_doc_)
	{
		rebuild_table();
		clear_editor_panels();
		return;
	}

	restore_filter_state(active_doc_->path());

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
	if (dict_doc)
	{
		filter_tree_view_->set_display_mode(filter_tree_view_t::display_mode_t::full);
		if (session_.dict_version() != last_annotation_version_)
		{
			rebuild_annotations();
			last_annotation_version_ = session_.dict_version();
		}
	}
	else if (dynamic_cast<plugin_document_t *>(active_doc_))
	{
		filter_tree_view_->set_display_mode(filter_tree_view_t::display_mode_t::empty);
		filter_tree_view_->setEnabled(false);
	}
	else
	{
		filter_tree_view_->set_display_mode(filter_tree_view_t::display_mode_t::all_only);
	}

	rebuild_table();
	clear_editor_panels();
}

void main_window_t::rebuild_table()
{
	if (!table_model_)
		return;

	if (!active_doc_)
	{
		table_display_->clear();
		editor_controller_.set_current_row(-1);
		clear_editor_panels();
		return;
	}

	if (dynamic_cast<plugin_document_t *>(active_doc_))
	{
		table_display_->clear();
		editor_controller_.set_current_row(-1);
		clear_editor_panels();
		return;
	}

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
	if (!dict_doc)
	{
		rebuild_table_yaml(active_doc_);
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

		if (row_filter_.has_query() && !row_filter_.matches(row))
			continue;

		filtered_status_counts[row.status]++;
	}

	std::vector<table_row_t> rows;
	for (const auto & row : raw_rows)
	{
		if (!status_filter_.empty() && status_filter_.count(row.status) == 0)
			continue;

		if (row_filter_.has_query() && !row_filter_.matches(row))
			continue;

		rows.push_back(row);
	}

	int total = target_doc->total_count();
	int translated = target_doc->translated_count();
	table_display_->apply_yaml(
	    std::move(rows), total, translated, target_doc->path(), filtered_status_counts, total_status_counts);
	editor_controller_.set_current_row(-1);
	clear_editor_panels();
}

void main_window_t::rebuild_table_dict(dict_document_t * dict_doc)
{
	const table_filter_params_t filter_params {
		type_filter_, filter_tree_view_->get_active_sub_types(), status_filter_, row_filter_, type_filter_solo_
	};

	auto result = build_filtered_rows(dict_doc->data(), filter_params);

	table_display_->apply(std::move(result), dict_doc->path(), dict_doc->kind());
	editor_controller_.set_current_row(-1);
	clear_editor_panels();
}

void main_window_t::on_row_selected(int row)
{
	if (row == editor_controller_.current_row())
		return;

	commit_current_edit();
	load_record(row);
}

void main_window_t::on_translation_changed()
{
	if (editor_controller_.is_loading())
		return;

	if (editor_controller_.current_row() < 0)
		return;

	if (!active_doc_)
		return;

	auto * yaml_doc = dynamic_cast<yaml_document_t *>(active_doc_);
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

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
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

	if (editor_controller_.current_row() < 0)
		return;

	const auto * row_data = table_model_->row_at(editor_controller_.current_row());
	if (!row_data)
		return;

	apply_translation_highlights(row_data);

	if (row_data->type == tools_t::rec_type_t::text)
	{
		const auto translation_text = editor_view_->translation_editor()->toPlainText().toStdString();
		book_preview_view_->set_html(row_data->old_text, translation_text);
	}
}

void main_window_t::apply_translation_highlights(const table_row_t * row_data)
{
	const auto annotations = glossary_.annotate(row_data->old_text, row_data->type);
	const auto current_text = editor_view_->translation_editor()->toPlainText().toLower();

	const highlight_config_t config { &annotations, false, highlight_sort_policy_t::hyperlink_first };
	auto highlights = find_annotation_highlights(current_text, config);

	extra_sel_translation_.annotations = build_highlight_selections(editor_view_->translation_editor(), highlights);

	extra_sel_translation_.grammar = grammar_check_->isChecked()
	                                     ? grammar_checker_.check(editor_view_->translation_editor(), row_data->type)
	                                     : QList<QTextEdit::ExtraSelection> {};
	apply_extra_selections(editor_view_->translation_editor(), extra_sel_translation_);
}

void main_window_t::commit_current_edit()
{
	if (editor_controller_.current_row() < 0)
		return;

	if (editor_controller_.is_loading())
		return;

	if (!editor_view_)
		return;

	if (!active_doc_)
		return;

	const auto & current_text = editor_view_->translation_editor()->toPlainText();
	if (current_text == editor_controller_.loaded_text())
		return;

	const auto * row_data = table_model_->row_at(editor_controller_.current_row());
	if (!row_data)
		return;

	std::string new_text_str;
	if (editor_view_->has_script_template())
	{
		new_text_str = editor_view_->reconstruct_script_line();

		const auto lines = current_text.split('\n');
		const size_t slot_count = editor_view_->script_slot_count();

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

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
	if (dict_doc)
	{
		commit_dict_edit(dict_doc, row_data, new_text_str);
		return;
	}

	commit_yaml_edit(active_doc_, row_data, new_text_str);
}

// irreducible: 3 params required — document pointer per extraction constraint + row context + new value
void main_window_t::commit_dict_edit(
    dict_document_t * dict_doc,
    const table_row_t * row_data,
    const std::string & new_text_str)
{
	const auto result = editor_controller_.commit(*dict_doc, *row_data, new_text_str);
	if (!result.success)
		return;

	if (result.propagated_count > 0)
	{
		statusBar()->showMessage(QString("Propagated to %1 entries").arg(result.propagated_count), 5000);
		table_model_->update_row(editor_controller_.current_row(), result.new_text, result.status);
		sync_propagated_rows(dict_doc);
		set_unsaved_changes(dict_doc->is_dirty());
		editor_controller_.set_loaded_text(editor_view_->translation_editor()->toPlainText());
		update_status_counts();
		return;
	}

	table_model_->update_row(editor_controller_.current_row(), result.new_text, result.status);
	set_unsaved_changes(dict_doc->is_dirty());
	editor_controller_.set_loaded_text(editor_view_->translation_editor()->toPlainText());
	update_status_counts();
}

// irreducible: 3 params required — document pointer per extraction constraint + row context + new value
void main_window_t::commit_yaml_edit(
    document_t * target_doc,
    const table_row_t * row_data,
    const std::string & new_text_str)
{
	target_doc->commit_edit(row_data->type, row_data->record_index, new_text_str);
	table_model_->update_row(editor_controller_.current_row(), new_text_str, status_t::in_progress);

	auto * yaml_doc = dynamic_cast<yaml_document_t *>(target_doc);
	if (yaml_doc)
		yaml_doc->save_tmp();

	set_unsaved_changes(target_doc->is_dirty());
	editor_controller_.set_loaded_text(editor_view_->translation_editor()->toPlainText());
	update_status_counts();
}

void main_window_t::sync_propagated_rows(dict_document_t * dict_doc)
{
	auto & data = dict_doc->data_mut();

	for (int i = 0; i < table_model_->rowCount(); ++i)
	{
		if (i == editor_controller_.current_row())
			continue;

		const auto * row = table_model_->row_at(i);
		if (!row)
			continue;

		auto chap_it = data.find(row->type);
		if (chap_it == data.end())
			continue;

		if (row->record_index >= chap_it->second.records.size())
			continue;

		const auto & record = chap_it->second.records[row->record_index];
		if (record.new_text != row->new_text || record.status != row->status)
			table_model_->update_row(i, record.new_text, record.status);
	}
}

// irreducible: sequential orchestrator — each step depends on prior state; no nesting to flatten
void main_window_t::load_record(int row)
{
	editor_controller_.set_loading(true);

	if (!table_model_ || !editor_view_)
	{
		editor_controller_.set_current_row(-1);
		editor_controller_.set_loading(false);
		return;
	}

	const auto * row_data = table_model_->row_at(row);
	if (!row_data)
	{
		load_record_clear(row);
		return;
	}

	const auto load_result = active_doc_ ? editor_controller_.load(*active_doc_, *row_data) : editor_load_result_t {};

	if (row_data->type == tools_t::rec_type_t::sctx || row_data->type == tools_t::rec_type_t::bnam)
		load_record_script(row_data);
	else
		load_record_plain(row_data);

	editor_view_->translation_editor()->setReadOnly(load_result.is_read_only);

	hl_original_->set_record_type(row_data->type);
	hl_adapted_->set_record_type(row_data->type);
	hl_translation_->set_record_type(row_data->type);

	const auto validation_result = byte_limit_validator_.validate(row_data->type, row_data->new_text);
	validation_view_->update_validation(validation_result);

	if (row_data->type == tools_t::rec_type_t::text)
		book_preview_view_->set_html(row_data->old_text, row_data->new_text);
	else
		book_preview_view_->clear();

	annotations_view_->update_annotations(
	    load_result.annotations, load_result.speaker_name, load_result.gender, load_result.enchantment);

	translation_tab_->set_source_text(row_data->old_text);

	if (!load_result.details.empty())
		editor_view_->set_details(load_result.details);
	else
		editor_view_->clear_details();

	const auto & annotations = load_result.annotations;
	const auto original_text_lower = editor_view_->original_view()->toPlainText().toLower();
	const auto translation_text_lower = editor_view_->translation_editor()->toPlainText().toLower();

	const highlight_config_t orig_config { &annotations, true, highlight_sort_policy_t::length_first };
	auto orig_highlights = find_annotation_highlights(original_text_lower, orig_config);

	extra_sel_original_.annotations = build_highlight_selections(editor_view_->original_view(), orig_highlights);
	extra_sel_original_.grammar.clear();
	extra_sel_original_.adapted_diff.clear();
	apply_extra_selections(editor_view_->original_view(), extra_sel_original_);

	const highlight_config_t trans_config { &annotations, false, highlight_sort_policy_t::length_first };
	auto trans_highlights = find_annotation_highlights(translation_text_lower, trans_config);

	extra_sel_translation_.annotations =
	    build_highlight_selections(editor_view_->translation_editor(), trans_highlights);
	extra_sel_translation_.grammar = grammar_check_->isChecked()
	                                     ? grammar_checker_.check(editor_view_->translation_editor(), row_data->type)
	                                     : QList<QTextEdit::ExtraSelection> {};
	extra_sel_translation_.adapted_diff.clear();
	apply_extra_selections(editor_view_->translation_editor(), extra_sel_translation_);

	extra_sel_adapted_.annotations.clear();
	extra_sel_adapted_.grammar.clear();
	extra_sel_adapted_.adapted_diff.clear();

	if (row_data->status == status_t::adapted && !load_result.details.empty())
	{
		extra_sel_adapted_.adapted_diff =
		    editor_view_->highlight_adapted_diff(row_data->new_text, load_result.details, false);
	}
	else if (row_data->status == status_t::changed && !load_result.details.empty())
	{
		extra_sel_adapted_.adapted_diff =
		    editor_view_->highlight_adapted_diff(row_data->old_text, load_result.details, true);
	}

	apply_extra_selections(editor_view_->details_view(), extra_sel_adapted_);

	const auto history = edit_history_.get_history(row_data->type, row_data->key_text);
	history_view_->update_history(history, !load_result.is_read_only);

	editor_controller_.set_loaded_text(editor_view_->translation_editor()->toPlainText());
	editor_controller_.set_current_row(row);

	auto cursor = editor_view_->translation_editor()->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor_view_->translation_editor()->setTextCursor(cursor);

	editor_controller_.set_loading(false);
}

void main_window_t::load_record_clear(int /*row*/)
{
	editor_view_->original_view()->clear();
	editor_view_->translation_editor()->clear();
	validation_view_->clear();
	annotations_view_->clear();
	history_view_->clear();
	book_preview_view_->clear();
	editor_controller_.set_current_row(-1);
	editor_controller_.set_loading(false);
}

void main_window_t::load_record_script(const table_row_t * row_data)
{
	editor_view_->load_script_entry(row_data->old_text, row_data->new_text);
	editor_view_->translation_editor()->set_block_multiline(true);

	if (row_data->status != status_t::untranslated)
		return;

	int line_count = editor_view_->script_slot_count();
	QString empty_lines;
	for (int i = 1; i < static_cast<int>(line_count); ++i)
		empty_lines += '\n';

	editor_view_->translation_editor()->setPlainText(empty_lines);
}

void main_window_t::load_record_plain(const table_row_t * row_data)
{
	editor_view_->original_view()->setPlainText(QString::fromStdString(row_data->old_text));
	editor_view_->clear_script_template();

	if (row_data->status == status_t::untranslated)
		editor_view_->translation_editor()->setPlainText(QString());
	else
		editor_view_->translation_editor()->setPlainText(QString::fromStdString(row_data->new_text));

	bool block_multiline =
	    (row_data->type == tools_t::rec_type_t::cell || row_data->type == tools_t::rec_type_t::dial ||
	     row_data->type == tools_t::rec_type_t::fnam);
	editor_view_->translation_editor()->set_block_multiline(block_multiline);
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
	if (!editor_view_)
		return;

	QTextOption opt;
	if (checked)
		opt.setFlags(QTextOption::ShowTabsAndSpaces);

	editor_view_->original_view()->document()->setDefaultTextOption(opt);
	editor_view_->translation_editor()->document()->setDefaultTextOption(opt);
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
	if (new_codepage == current_codepage_)
		return;

	current_codepage_ = new_codepage;
	session_.set_codepage(new_codepage);
	byte_limit_validator_.set_codepage(new_codepage);
	config_.encoding_index = index;
	save_config();

	statusBar()->showMessage(
	    "Encoding changed. Open documents keep their original encoding until "
	    "re-opened.",
	    5000);
}

void main_window_t::rebuild_annotations()
{
	std::vector<dict_source_t> sources;
	for (auto * dict_doc : session_.all_dicts())
		sources.push_back({ &dict_doc->data(), dict_doc->path() });

	glossary_.rebuild(sources);
}

void main_window_t::save_current_filter_state()
{
	if (!active_doc_)
		return;

	filter_state_t state;
	state.type_filter = type_filter_;
	state.sub_type_filter = filter_tree_view_->get_active_sub_types();
	state.status_filter = status_filter_;
	state.type_filter_solo = type_filter_solo_;
	filter_states_[active_doc_->path()] = std::move(state);
}

void main_window_t::restore_filter_state(const std::string & path)
{
	auto it = filter_states_.find(path);
	if (it != filter_states_.end())
	{
		type_filter_ = it->second.type_filter;
		status_filter_ = it->second.status_filter;
		type_filter_solo_ = it->second.type_filter_solo;
		filter_tree_view_->set_active_types(it->second.type_filter);
		filter_tree_view_->set_active_sub_types(it->second.sub_type_filter);
	}
	else
	{
		type_filter_ = {
			tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
			tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
			tools_t::rec_type_t::indx, tools_t::rec_type_t::sctx,
		};
		status_filter_.clear();
		type_filter_solo_ = false;
		filter_tree_view_->set_active_types(type_filter_);
		filter_tree_view_->set_active_sub_types({});
	}
}

void main_window_t::rebuild_sidebar()
{
	std::string active_path;
	if (active_doc_)
		active_path = active_doc_->path();

	auto model = build_render_model(file_list_, session_, active_path);
	sidebar_->set_model(model);
}

void main_window_t::update_sidebar_item(const std::string & path)
{
	const auto * fe = file_list_.get(path);
	if (!fe)
		return;

	const auto * doc = session_.find(path);
	const bool is_loaded = (doc != nullptr);
	const bool is_dirty = doc && doc->is_dirty();
	sidebar_->update_item_text(fe->path, derive_display_name(*fe, is_loaded, is_dirty));
}

void main_window_t::update_annotations()
{
	if (editor_controller_.current_row() < 0)
		return;

	const auto * row_data = table_model_->row_at(editor_controller_.current_row());
	if (!row_data)
		return;

	const auto annotations = glossary_.annotate(row_data->old_text, row_data->type);

	std::string speaker_name;
	std::string gender_str;
	std::string enchantment_str;

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
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

	annotations_view_->update_annotations(annotations, speaker_name, gender_str, enchantment_str);
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
	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
	if (!dict_doc)
		return;

	const table_filter_params_t filter_params {
		type_filter_, filter_tree_view_->get_active_sub_types(), status_filter_, row_filter_, type_filter_solo_
	};

	auto result = build_filtered_rows(dict_doc->data(), filter_params);

	size_t total = 0;
	size_t total_translated = 0;
	for (const auto & [t, c] : result.counts.type_counts)
		total += c;
	for (const auto & [t, c] : result.counts.translated_counts)
		total_translated += c;

	filter_tree_view_->update_counts(result.counts.type_counts, result.counts.translated_counts);
	filter_tree_view_->update_sub_type_counts(
	    result.counts.sub_type_total_counts, result.counts.sub_type_translated_counts);
	filter_tree_view_->set_total_count(total_translated, total);
	status_filter_view_->update_counts(result.counts.filtered_status_counts, result.counts.total_status_counts);
}

void main_window_t::update_validation()
{
	if (editor_controller_.current_row() < 0)
		return;

	const auto * row_data = table_model_->row_at(editor_controller_.current_row());
	if (!row_data)
		return;

	const auto current_text = editor_view_->translation_editor()->toPlainText().toStdString();
	const auto result = byte_limit_validator_.validate(row_data->type, current_text);
	validation_view_->update_validation(result);
}

void main_window_t::scan_spell_dictionaries()
{
	const auto app_dir = QCoreApplication::applicationDirPath();
	QDir dict_dir(app_dir + "/dictionaries");

	if (!dict_dir.exists())
		return;

	const auto aff_files = dict_dir.entryList({ "*.aff" }, QDir::Files);
	int index = 1;
	for (const auto & aff_file : aff_files)
	{
		auto base_name = aff_file;
		base_name.chop(4);

		const auto dic_path = dict_dir.filePath(base_name + ".dic");
		if (!QFile::exists(dic_path))
			continue;

		auto * act = spelling_menu_->addAction(base_name);
		act->setCheckable(true);
		act->setData(index);
		spelling_group_->addAction(act);
		++index;
	}
}

void main_window_t::on_spell_lang_changed(int index)
{
	if (index <= 0)
	{
		hl_translation_->set_spell_checker(nullptr);
		return;
	}

	const auto & actions = spelling_group_->actions();
	if (index < 0 || index >= actions.size())
		return;

	const auto lang_name = actions.at(index)->text();
	const auto app_dir = QCoreApplication::applicationDirPath();
	const auto aff_path = app_dir + "/dictionaries/" + lang_name + ".aff";
	const auto dic_path = app_dir + "/dictionaries/" + lang_name + ".dic";

	if (!spell_checker_.load(aff_path.toStdString(), dic_path.toStdString()))
		return;

	hl_translation_->set_spell_checker(&spell_checker_);
}

void main_window_t::load_config()
{
	const auto path = QCoreApplication::applicationDirPath() + "/yTranslator.ini";
	config_.load(path.toStdString());

	move(config_.window_x, config_.window_y);
	resize(config_.window_w, config_.window_h);

	if (config_.window_maximized)
		showMaximized();

	if (config_.encoding_index >= 0 && config_.encoding_index < encoding_group_->actions().size())
		encoding_group_->actions().at(config_.encoding_index)->setChecked(true);

	constexpr codepage_t codepages_table[] = {
		codepage_t::windows_1250,
		codepage_t::windows_1251,
		codepage_t::windows_1252,
	};
	if (config_.encoding_index >= 0 && config_.encoding_index < 3)
		current_codepage_ = codepages_table[config_.encoding_index];

	session_.set_codepage(current_codepage_);

	translation_tab_->set_language_index(config_.translation_language_index);

	sidebar_toggle_->setChecked(config_.sidebar_visible);
	bottom_panel_toggle_->setChecked(config_.bottom_visible);

	if (config_.split_ratio > 0.0f)
		editor_view_->set_split_ratio(config_.split_ratio);

	std::vector<int> col_widths;
	for (auto w : config_.column_widths)
		col_widths.push_back(static_cast<int>(w));
	table_view_->set_column_widths(col_widths);

	file_list_.scan_roots(config_.workspace_roots);
	scan_workspace();

	if (!config_.active_dict_path.empty())
	{
		auto * doc = session_.open(config_.active_dict_path);
		if (doc)
			switch_document(doc);
	}

	rebuild_sidebar();
	rebuild_table();

	if (config_.spell_lang_index > 0 && config_.spell_lang_index < spelling_group_->actions().size())
		spelling_group_->actions().at(config_.spell_lang_index)->setChecked(true);

	on_spell_lang_changed(config_.spell_lang_index);

	update_watcher_paths();
}

void main_window_t::save_config()
{
	config_.window_x = pos().x();
	config_.window_y = pos().y();
	config_.window_w = size().width();
	config_.window_h = size().height();
	config_.window_maximized = isMaximized();

	config_.encoding_index =
	    encoding_group_->checkedAction() ? encoding_group_->actions().indexOf(encoding_group_->checkedAction()) : 2;
	config_.spell_lang_index =
	    spelling_group_->checkedAction() ? spelling_group_->actions().indexOf(spelling_group_->checkedAction()) : 0;

	config_.sidebar_visible = sidebar_toggle_->isChecked();
	config_.bottom_visible = bottom_panel_toggle_->isChecked();

	config_.split_ratio = static_cast<float>(editor_view_->get_split_ratio());

	const auto col_widths = table_view_->get_column_widths();
	for (size_t i = 0; i < col_widths.size() && i < config_.column_widths.size(); ++i)
		config_.column_widths[i] = static_cast<float>(col_widths[i]);

	config_.active_dict_index = -1; // deprecated — use active_dict_path
	config_.active_dict_path = active_doc_ ? active_doc_->path() : std::string {};
	config_.translation_language_index = translation_tab_->language_index();
	config_.workspace_roots = file_list_.get_roots();

	const auto path = QCoreApplication::applicationDirPath() + "/yTranslator.ini";
	config_.save(path.toStdString());
}

// irreducible: switch dispatch for 5 operation types; each branch contains distinct dialog/executor calls
void main_window_t::on_plugin_operation(const std::string & plugin_path_arg, plugin_op_t op)
{
	const auto plugin_path = plugin_path_arg;
	const auto encoding = current_codepage_;

	auto path_sep = plugin_path.find_last_of("/\\");
	auto plugin_dir = path_sep != std::string::npos ? plugin_path.substr(0, path_sep) : std::string {};
	plugin_dir = string_utils::normalize_path(plugin_dir);

	const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";

	if (op == plugin_op_t::convert || op == plugin_op_t::create_plugin)
		executor_.set_output_dir(plugin_dir);
	else
		executor_.set_output_dir(workspace_dir);

	if (session_.has_any_unsaved())
	{
		auto answer = QMessageBox::question(
		    this,
		    "Unsaved Changes",
		    "Some dictionaries have unsaved changes. Save before proceeding?",
		    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

		if (answer == QMessageBox::Cancel)
			return;

		if (answer == QMessageBox::Yes)
			session_.save_all();
	}

	operation_executor_t::result_t result;

	switch (op)
	{
	case plugin_op_t::make_dict:
	{
		result = executor_.make_dict(plugin_path, encoding);
		break;
	}
	case plugin_op_t::make_dict_with_base:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, config_.last_merge_order, this);
		dialog.setWindowTitle("Select Dictionaries");
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		config_.last_merge_order = selected;

		for (const auto & sel_path : selected)
			session_.open(sel_path);

		dict_merger_t merger(selected);
		result = executor_.make_dict_with_base(plugin_path, merger.get_dict(), encoding);
		break;
	}
	case plugin_op_t::make_base:
	{
		auto params = show_make_base_dialog(plugin_path);
		if (!params.has_value())
			return;

		result = executor_.make_base(
		    plugin_path, params->native_path, params->foreign_lang, params->native_lang, nullptr, params->base_mode);
		break;
	}
	case plugin_op_t::convert:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, config_.last_merge_order, this);
		dialog.setWindowTitle("Select Dictionaries for Convert");
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		config_.last_merge_order = selected;

		for (const auto & sel_path : selected)
			session_.open(sel_path);

		result = executor_.convert(plugin_path, selected, encoding);
		break;
	}
	case plugin_op_t::create_plugin:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, config_.last_merge_order, this);
		dialog.setWindowTitle("Select Dictionaries for Create");
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		config_.last_merge_order = selected;

		for (const auto & sel_path : selected)
			session_.open(sel_path);

		result = executor_.create_plugin(plugin_path, selected, encoding);
		break;
	}
	}

	log_operation_result(plugin_path, op, result);

	if (!result.output_path.empty())
	{
		auto norm_output = string_utils::normalize_path(result.output_path);

		if (active_doc_ && active_doc_->path() == norm_output)
			active_doc_ = nullptr;

		session_.close(result.output_path);
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

	for (const auto * entry : file_list_.all())
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
	const auto * native_entry = file_list_.get(native_path);

	std::string foreign_lang;
	const auto * foreign_entry = file_list_.get(plugin_path);
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

	log_view_->append_log(op_name, result.log_text);
	record_tabs_->setCurrentWidget(log_view_);
}

// Indivisible batch pattern: shared_ptr + QTimer + lambda
void main_window_t::start_batch_translation(dict_document_t * dict_doc)
{
	translation_tab_->set_translate_all_enabled(false);
	translation_tab_->append_log("[info] collecting untranslated entries...\n");

	struct batch_state_t
	{
		std::vector<std::pair<tools_t::rec_type_t, size_t>> work_items;
		size_t current = 0;
		int translated_count = 0;
		int glossary_count = 0;
		int error_count = 0;
	};

	auto state = std::make_shared<batch_state_t>();
	auto * provider = translation_tab_->ct2_provider();

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
		translation_tab_->append_log("[info] no untranslated entries found\n");
		translation_tab_->set_translate_all_enabled(true);
		return;
	}

	translation_tab_->append_log("[info] translating " + std::to_string(state->work_items.size()) + " entries\n");

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

			if (editor_controller_.current_row() >= 0)
				load_record(editor_controller_.current_row());

			translation_tab_->append_log(
			    "[info] done: translated=" + std::to_string(state->translated_count) + " glossary=" +
			    std::to_string(state->glossary_count) + " errors=" + std::to_string(state->error_count) + "\n");
			translation_tab_->set_translate_all_enabled(true);
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
			translation_tab_->append_log(
			    "[error] \"" + record.old_text.substr(0, 40) + "...\" -> " + result.error + "\n");
			++state->error_count;
			++state->current;
			return;
		}

		auto glossary_applied = glossary_.apply_glossary(result.text);
		bool had_glossary = (glossary_applied != result.text);

		record.new_text = glossary_applied;
		record.status = status_t::model;
		dict_doc->modified_records_insert(type, idx);
		++state->translated_count;

		for (int row = 0; row < table_model_->rowCount(); ++row)
		{
			const auto * r = table_model_->row_at(row);
			if (r && r->type == type && r->record_index == idx)
			{
				table_model_->update_row(row, record.new_text, record.status);
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
	file_list_.remove(path);
	rebuild_sidebar();
}

void main_window_t::scan_workspace()
{
	const auto workspace = (QCoreApplication::applicationDirPath() + "/workspace").toStdString();
	QDir().mkpath(QString::fromStdString(workspace));

	std::vector<std::string> roots;
	roots.push_back(workspace);
	for (const auto & r : config_.workspace_roots)
	{
		if (r != workspace)
			roots.push_back(r);
	}

	file_list_.scan_roots(roots);

	bool any_loaded = false;
	for (const auto * entry : file_list_.all())
	{
		if (!entry->is_workspace)
			continue;

		if (entry->type != file_type_t::base_dict && entry->type != file_type_t::user_dict)
			continue;

		if (session_.find(entry->path))
			continue;

		if (session_.open(entry->path))
			any_loaded = true;
	}

	if (any_loaded)
	{
		rebuild_annotations();
		last_annotation_version_ = session_.dict_version();
	}

	rebuild_sidebar();
}

void main_window_t::update_watcher_paths()
{
	const auto current = fs_watcher_->directories();
	if (!current.isEmpty())
		fs_watcher_->removePaths(current);

	QStringList paths;

	const auto workspace = QCoreApplication::applicationDirPath() + "/workspace";
	add_directory_recursive(paths, workspace);

	for (const auto & root : config_.workspace_roots)
		add_directory_recursive(paths, QString::fromStdString(root));

	if (!paths.isEmpty())
		fs_watcher_->addPaths(paths);
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
	for (const auto & p : config_.last_merge_order)
		saved_order_set.insert(normalize(p));
	bool use_saved = !saved_order_set.empty();

	for (const auto * dict_doc : session_.all_dicts())
	{
		auto norm = normalize(dict_doc->path());
		if (!seen.insert(norm).second)
			continue;

		bool pre = use_saved ? (saved_order_set.count(norm) > 0) : matches_source_dir(norm, target);

		std::string root_path;
		std::string subfolder;
		const auto * fe = file_list_.get(dict_doc->path());
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

	for (const auto * fe : file_list_.all())
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
	const auto * entry = file_list_.get(path);
	if (!entry)
		return;

	auto norm_path = string_utils::normalize_path(path);

	if (active_doc_ && active_doc_->path() == norm_path)
		return;

	auto * doc = session_.open(path);
	if (!doc)
		return;

	switch_document(doc);
	rebuild_sidebar();
}

void main_window_t::on_operation_requested(const std::string & path, plugin_op_t op)
{
	const auto * entry = file_list_.get(path);
	if (!entry)
		return;

	const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";
	const auto output_dir = derive_output_dir(*entry, workspace_dir);
	executor_.set_output_dir(output_dir);

	on_plugin_operation(path, op);
}

void main_window_t::on_save_requested(const std::string & path)
{
	auto norm_path = string_utils::normalize_path(path);

	if (active_doc_ && active_doc_->path() == norm_path)
	{
		commit_current_edit();
		active_doc_->save();
		update_sidebar_item(active_doc_->path());

		log_view_->append_log("save", "saved \"" + active_doc_->path() + "\"\r\n");

		if (!session_.has_any_unsaved())
			set_unsaved_changes(false);

		return;
	}

	auto * doc = session_.find(path);
	if (!doc)
		return;

	doc->save();
	update_sidebar_item(doc->path());

	log_view_->append_log("save", "saved \"" + doc->path() + "\"\r\n");

	if (!session_.has_any_unsaved())
		set_unsaved_changes(false);
}

void main_window_t::on_unload_requested(const std::string & path)
{
	auto * doc = session_.find(path);
	if (!doc)
	{
		file_list_.remove(path);
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

	if (active_doc_ && active_doc_->path() == doc->path())
		switch_document(nullptr);

	session_.close(path);

	auto norm = string_utils::normalize_path(path);
	filter_states_.erase(norm);

	rebuild_annotations();
	last_annotation_version_ = session_.dict_version();
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
	if (active_doc_ && active_doc_->path() == norm_del_path)
		switch_document(nullptr);

	session_.close(path);
	filter_states_.erase(norm_del_path);
	rebuild_annotations();
	last_annotation_version_ = session_.dict_version();
	file_list_.remove(path);
	rebuild_sidebar();
	rebuild_table();
	scan_workspace();
}

void main_window_t::closeEvent(QCloseEvent * event)
{
	if (session_.has_any_unsaved())
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
			session_.save_all();
	}

	commit_current_edit();
	if (auto * yaml_doc = dynamic_cast<yaml_document_t *>(active_doc_))
		yaml_doc->save_tmp();

	save_config();
	QMainWindow::closeEvent(event);
}
