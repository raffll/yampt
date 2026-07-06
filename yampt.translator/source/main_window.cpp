#include "main_window.hpp"
#include <utility/app_logger.hpp>
#include "dialog/dict_selection_dialog.hpp"
#include "dialog/find_replace_dialog.hpp"
#include "dialog/first_run_dialog.hpp"
#include "dialog/make_base_dialog.hpp"
#include "dialog/merge_dialog.hpp"
#include "dialog/settings/translator_settings_dialog.hpp"
#include "dialog/spell_context_menu.hpp"
#include "highlighter/editor_highlighter.hpp"
#include "highlighter/glossary_highlighter.hpp"
#include "highlighter/grammar_checker.hpp"
#include "highlighter/topic_highlighter.hpp"
#include "model/dict_document.hpp"
#include "model/plugin_document.hpp"
#include "model/table_builder.hpp"
#include "model/yaml_document.hpp"
#include "translator/ctranslate2_translator.hpp"
#include "view/annotations_view.hpp"
#include "view/book_preview_view.hpp"
#include "view/display_name.hpp"
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
#include <theme_system.hpp>
#include <unordered_map>
#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
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
#include <QToolButton>
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
		rec_type_t::cell, rec_type_t::dial, rec_type_t::info, rec_type_t::fnam,
		rec_type_t::text, rec_type_t::gmst, rec_type_t::desc, rec_type_t::rnam,
		rec_type_t::indx, rec_type_t::sctx,
	};

	setup_menu_bar();
	setup_toolbar();
	setup_central_widget();
	setup_sidebar();
	setup_editor_panel();
	setup_status_bar();
	setup_table_display();

	m_sidebar_controller = std::make_unique<sidebar_controller_t>(
	    sidebar_controller_deps_t { m_session,
	                                m_file_list,
	                                *m_workspace_watcher,
	                                *m_sidebar,
	                                *m_log_view,
	                                m_filter_states,
	                                m_last_annotation_version,
	                                m_active_doc,
	                                this,
	                                { [this](document_t * doc) { switch_document(doc); },
	                                  [this]() { rebuild_annotations(); },
	                                  [this]() { save_config(); },
	                                  [this](bool dirty) { set_unsaved_changes(dirty); } } });

	m_plugin_ops_controller = std::make_unique<plugin_operations_controller_t>(
	    plugin_operations_deps_t { m_session,
	                               m_file_list,
	                               m_settings,
	                               m_executor,
	                               *m_log_view,
	                               *m_translation_tab,
	                               *m_record_tabs,
	                               m_current_codepage,
	                               this,
	                               { [this]() { m_sidebar_controller->scan_workspace(); },
	                                 [this](const std::string & path) { return show_make_base_dialog(path); },
	                                 [this](dict_document_t * doc) { start_batch_translation(doc); } } });

	m_record_display_controller =
	    std::make_unique<record_display_controller_t>(record_display_deps_t { *m_editor_view,
	                                                                          *m_table_model,
	                                                                          m_editor_controller,
	                                                                          m_glossary,
	                                                                          m_grammar_checker,
	                                                                          m_byte_limit_validator,
	                                                                          m_edit_history,
	                                                                          *m_annotations_view,
	                                                                          *m_history_view,
	                                                                          *m_book_preview_view,
	                                                                          *m_validation_view,
	                                                                          *m_translation_tab,
	                                                                          m_extra_sel_original,
	                                                                          m_extra_sel_adapted,
	                                                                          m_extra_sel_translation,
	                                                                          *m_grammar_check });

	connect_menu_signals();
	connect_sidebar_signals();
	connect_editor_signals();
	connect_search_signals();

	connect(
	    &theme_system_t::instance(),
	    &theme_system_t::theme_changed,
	    this,
	    [this](theme_t)
	{
		theme_system_t::instance().apply_to_application();
		m_status_filter_view->refresh_theme();
		m_table_view->viewport()->update();
		m_sidebar->update();
	});

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
	m_dict_ops_controller->on_merge();
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

	if (row_data->type == rec_type_t::text)
	{
		const auto translation_text = m_editor_view->translation_editor()->toPlainText().toStdString();
		m_book_preview_view->set_html(row_data->old_text, translation_text);
	}
}

void main_window_t::apply_translation_highlights(const table_row_t * row_data)
{
	m_record_display_controller->apply_translation_highlights(row_data);
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
		const auto result = m_editor_controller.commit_dict_full(*dict_doc, *row_data, new_text_str);
		if (!result.base_result.success)
			return;

		m_table_model->update_row(
		    m_editor_controller.current_row(), result.base_result.new_text, result.base_result.status);

		if (result.base_result.propagated_count > 0)
		{
			statusBar()->showMessage(
			    QString("Propagated to %1 entries").arg(result.base_result.propagated_count), 5000);
			m_editor_controller.sync_propagated_rows(*m_table_model, *dict_doc);
		}

		set_unsaved_changes(dict_doc->is_dirty());
		m_editor_controller.set_loaded_text(m_editor_view->translation_editor()->toPlainText());
		update_status_counts();
		return;
	}

	m_editor_controller.commit_yaml(*m_active_doc, *row_data, new_text_str);
	m_table_model->update_row(m_editor_controller.current_row(), new_text_str, status_t::in_progress);
	set_unsaved_changes(m_active_doc->is_dirty());
	m_editor_controller.set_loaded_text(m_editor_view->translation_editor()->toPlainText());
	update_status_counts();
}

void main_window_t::load_record(int row)
{
	m_record_display_controller->load_record(row, m_active_doc);

	const auto * row_data = m_table_model->row_at(row);
	if (row_data)
	{
		m_hl_original->set_record_type(row_data->type);
		m_hl_adapted->set_record_type(row_data->type);
		m_hl_translation->set_record_type(row_data->type);
	}
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
	m_shortcuts_controller->copy_original();
}

void main_window_t::shortcut_commit_status(status_t new_status)
{
	m_shortcuts_controller->commit_status(new_status);
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
			rec_type_t::cell, rec_type_t::dial, rec_type_t::info, rec_type_t::fnam,
			rec_type_t::text, rec_type_t::gmst, rec_type_t::desc, rec_type_t::rnam,
			rec_type_t::indx, rec_type_t::sctx,
		};
		m_status_filter.clear();
		m_type_filter_solo = false;
		m_filter_tree_view->set_active_types(m_type_filter);
		m_filter_tree_view->set_active_sub_types({});
	}
}

void main_window_t::rebuild_sidebar()
{
	m_sidebar_controller->rebuild_sidebar();
}

void main_window_t::update_sidebar_item(const std::string & path)
{
	m_sidebar_controller->update_sidebar_item(path);
}

void main_window_t::update_annotations()
{
	m_record_display_controller->update_annotations(m_active_doc);
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
	m_record_display_controller->update_validation();
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

	update_watcher_roots();
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

void main_window_t::on_plugin_operation(const std::string & plugin_path_arg, plugin_op_t op)
{
	m_plugin_ops_controller->on_plugin_operation(plugin_path_arg, op);

	if (m_active_doc && !m_session.find(m_active_doc->path()))
		m_active_doc = nullptr;
}

std::optional<make_base_params_t> main_window_t::show_make_base_dialog(const std::string & plugin_path)
{
	make_base_dialog_t dialog(m_file_list, m_settings, plugin_path, this);
	if (dialog.exec() != QDialog::Accepted)
		return std::nullopt;

	return dialog.result();
}

void main_window_t::start_batch_translation(dict_document_t * dict_doc)
{
	m_dict_ops_controller->start_batch_translation(dict_doc);
}

void main_window_t::on_plugin_unload(const std::string & path)
{
	m_file_list.remove(path);
	rebuild_sidebar();
}

void main_window_t::scan_workspace()
{
	m_sidebar_controller->scan_workspace();
}

void main_window_t::update_watcher_roots()
{
	m_sidebar_controller->update_watcher_roots();
}

void main_window_t::on_item_clicked(const std::string & path)
{
	m_sidebar_controller->on_item_clicked(path);
}

void main_window_t::on_operation_requested(const std::string & path, plugin_op_t op)
{
	const auto * entry = m_file_list.get(path);
	if (!entry)
		return;

	on_plugin_operation(path, op);
}

void main_window_t::on_save_requested(const std::string & path)
{
	commit_current_edit();
	m_sidebar_controller->on_save_requested(path);
}

void main_window_t::on_unload_requested(const std::string & path)
{
	m_sidebar_controller->on_unload_requested(path);
	rebuild_table();
}

void main_window_t::on_delete_requested(const std::string & path)
{
	m_sidebar_controller->on_delete_requested(path);
	rebuild_table();
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
