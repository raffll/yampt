#include <resource_paths.hpp>
#include "main_window.hpp"
#include "dialog/dict_selection_dialog.hpp"
#include "dialog/first_run_dialog.hpp"
#include "dialog/make_base_dialog.hpp"
#include "dialog/settings/translator_settings_dialog.hpp"
#include "editor/commit_orchestrator.hpp"
#include "highlighter/editor_highlighter.hpp"
#include "highlighter/grammar_checker.hpp"
#include "model/dict_document.hpp"
#include "model/yaml_document.hpp"
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
#include <translation_example.hpp>
#include <translator/translation_example_ops.hpp>
#include <utility/language_config.hpp>
#include <utility/string_utils.hpp>
#include <algorithm>
#include <filesystem>
#include <map>
#include <theme_system.hpp>
#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextDocument>
#include <QTextOption>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

main_window_t::main_window_t(QWidget * parent)
    : QMainWindow(parent)
    , m_session(m_current_codepage)
    , m_editor_controller(m_glossary)
{
	setWindowTitle(tr("yTranslator"));
	resize(1280, 720);
	setMinimumSize(800, 600);

	m_type_filter = {
		rec_type_t::cell, rec_type_t::dial, rec_type_t::info, rec_type_t::fnam, rec_type_t::text,
		rec_type_t::gmst, rec_type_t::desc, rec_type_t::rnam, rec_type_t::indx, rec_type_t::sctx,
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
	                               this,
	                               { [this]() { m_sidebar_controller->scan_workspace(); },
	                                 [this](const std::string & path) { return show_make_base_dialog(path); } } });

	m_dict_ops_controller = std::make_unique<dict_operations_controller_t>(
	    dict_operations_deps_t { m_session,
	                             *m_log_view,
	                             this,
	                             m_edit_history,
	                             [this]() { m_sidebar_controller->scan_workspace(); },
	                             [this](document_t * doc) { switch_document(doc); },
	                             [this]() { rebuild_sidebar(); },
	                             [this]() { return dynamic_cast<dict_document_t *>(m_active_doc); },
	                             [this]() { rebuild_table(); } });

	m_record_display_controller =
	    std::make_unique<record_display_controller_t>(record_display_deps_t { *m_editor_view,
	                                                                          *m_table_model,
	                                                                          m_editor_controller,
	                                                                          m_glossary,
	                                                                          m_inflection_store,
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

	m_shortcuts_controller =
	    std::make_unique<shortcuts_controller_t>(shortcuts_deps_t { m_editor_controller,
	                                                                *m_table_model,
	                                                                [this]() -> document_t * { return m_active_doc; },
	                                                                [this](bool dirty) { set_unsaved_changes(dirty); },
	                                                                [this]() { update_status_counts(); },
	                                                                [this](int row) { load_record(row); } });

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

			const auto languages =
			    language_config::load(resource_paths::languages_file());

			const auto * native_lang = language_config::find_by_code(languages, native);
			const int encoding_index = native_lang ? codepage_to_index(native_lang->codepage) : 2;
			m_settings.set_encoding_index(encoding_index);
			on_encoding_changed(encoding_index);

			const auto dict_dir = resource_paths::dictionaries_dir();
			auto set_spell_paths = [&](const std::string & lang_code, bool is_native)
			{
				const auto prefix = language_config::resolve_dictionary_prefix(languages, lang_code);
				if (prefix.empty())
					return;

				const auto aff_path = dict_dir + prefix + ".aff";
				const auto dic_path = dict_dir + prefix + ".dic";

				if (!std::filesystem::exists(aff_path) || !std::filesystem::exists(dic_path))
					return;

				if (is_native)
				{
					m_settings.set_spell_aff_path(aff_path);
					m_settings.set_spell_dic_path(dic_path);
				}
				else
				{
					m_settings.set_partial_dict_aff_path(aff_path);
					m_settings.set_partial_dict_dic_path(dic_path);
				}
			};

			set_spell_paths(native, true);
			set_spell_paths(foreign, false);

			save_config();
		}
	}
}

void main_window_t::set_unsaved_changes(bool dirty)
{
	if (m_has_unsaved_changes == dirty)
		return;

	m_has_unsaved_changes = dirty;
	setWindowTitle(m_has_unsaved_changes ? tr("yTranslator *") : tr("yTranslator"));
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
	const bool key_column_present = !m_table_model || m_table_model->columns().contains(col_key);
	if (m_search_col_key->isChecked() && key_column_present)
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
	m_editor_view->translation_editor()->setReadOnly(true);
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

	if (m_active_doc->kind() == document_kind_t::dict)
	{
		m_filter_tree_view->set_display_mode(filter_tree_view_t::display_mode_t::full);
		m_status_filter_view->set_visible_statuses(m_active_doc->supported_statuses());
		if (m_session.dict_version() != m_last_annotation_version)
		{
			rebuild_annotations();
			m_last_annotation_version = m_session.dict_version();
		}
	}
	else if (m_active_doc->kind() == document_kind_t::plugin)
	{
		m_filter_tree_view->set_display_mode(filter_tree_view_t::display_mode_t::empty);
		m_filter_tree_view->setEnabled(false);
		m_status_filter_view->set_visible_statuses(m_active_doc->supported_statuses());
	}
	else
	{
		m_filter_tree_view->set_display_mode(filter_tree_view_t::display_mode_t::all_only);
		m_status_filter_view->set_visible_statuses(m_active_doc->supported_statuses());
	}

	m_table_view->set_context_menu_enabled(m_active_doc->permissions().status_changeable);

	rebuild_table();
	clear_editor_panels();
}

void main_window_t::rebuild_table()
{
	if (!m_table_model)
		return;

	m_table_model->set_editable(true);

	if (!m_active_doc)
	{
		m_table_display->clear();
		m_editor_controller.set_current_row(-1);
		clear_editor_panels();
		return;
	}

	if (m_active_doc->kind() == document_kind_t::plugin)
	{
		m_table_display->clear();
		m_editor_controller.set_current_row(-1);
		clear_editor_panels();
		return;
	}

	m_table_model->set_columns(m_active_doc->kind());
	if (m_table_view)
		m_table_view->refresh_column_layout();

	auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
	if (dict_doc)
	{
		rebuild_table_dict(dict_doc);
		return;
	}

	rebuild_table_yaml(m_active_doc);
}

void main_window_t::rebuild_table_yaml(document_t * target_doc)
{
	m_table_model->set_editable(target_doc->permissions().inline_editable);

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

	m_table_display->apply(std::move(result), dict_doc->path(), dict_doc->dict_kind());
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

	const auto current_text = m_editor_view->translation_editor()->toPlainText();
	const bool text_matches_loaded = (current_text == m_editor_controller.loaded_text());

	if (!text_matches_loaded)
	{
		if (!m_active_doc->is_dirty())
		{
			m_active_doc->set_dirty(true);
			update_sidebar_item(m_active_doc->path());
		}

		set_unsaved_changes(true);
	}

	update_validation();

	const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
	if (!row_data)
		return;

	apply_translation_highlights(row_data);

	if (row_data->type == rec_type_t::text)
	{
		const auto translation_text = m_editor_view->translation_editor()->toPlainText().toStdString();
		m_book_preview_view->set_html(row_data->old_text, translation_text);
	}
	else if (row_data->type == rec_type_t::sctx || row_data->type == rec_type_t::bnam) {}
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
			    tr("Warning: expected %1 strings, got %2").arg(slot_count).arg(lines.size()), 5000);
		}
	}
	else
	{
		new_text_str = current_text.toStdString();
	}

	const auto intent = m_editor_controller.take_pending_status().value_or(status_t::in_progress);

	const auto commit_output = commit_orchestrator::execute(
	    { *row_data, m_editor_controller.loaded_text().toStdString(), new_text_str, intent },
	    *m_active_doc,
	    m_edit_history,
	    m_byte_limit_validator,
	    m_glossary);

	if (!commit_output.result.success)
		return;

	m_table_model->update_row(
	    m_editor_controller.current_row(), commit_output.result.new_text, commit_output.result.status);

	if (commit_output.result.propagated_count > 0)
	{
		statusBar()->showMessage(tr("Propagated to %1 entries").arg(commit_output.result.propagated_count), 5000);

		auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
		if (dict_doc)
			m_editor_controller.sync_propagated_rows(*m_table_model, *dict_doc);
	}

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
		opt.setFlags(QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators);

	m_editor_view->original_view()->document()->setDefaultTextOption(opt);
	m_editor_view->translation_editor()->document()->setDefaultTextOption(opt);
}

void main_window_t::on_encoding_changed(int index)
{
	if (index < 0 || index >= static_cast<int>(std::size(supported_codepages)))
		return;

	const auto new_codepage = index_to_codepage(index);
	if (new_codepage == m_current_codepage)
		return;

	m_current_codepage = new_codepage;
	m_session.set_codepage(new_codepage);
	m_byte_limit_validator.set_codepage(new_codepage);
	m_settings.set_encoding_index(index);
	save_config();

	statusBar()->showMessage(
	    tr("Encoding changed. Open documents keep their original encoding until re-opened."), 5000);
}

void main_window_t::on_open_settings()
{
	const auto dict_dir = resource_paths::dictionaries_dir();
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
		on_spell_lang_changed();
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
		m_copy_original_action->setToolTip(tr("Copy original text to translation (F8)"));
		m_copy_original_action->setShortcutContext(Qt::WindowShortcut);
		addAction(m_copy_original_action);
		connect(m_copy_original_action, &QAction::triggered, this, [this]() { shortcut_copy_original(); });
	}

	if (!m_set_in_progress_action)
	{
		m_set_in_progress_action = new QAction(this);
		m_set_in_progress_action->setToolTip(tr("Set status to In Progress (F9)"));
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
		m_set_translated_action->setToolTip(tr("Set status to Translated (F10)"));
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

	if (m_settings_action)
		m_settings_action->setShortcut(resolve("settings", "Ctrl+,"));

	if (m_escape_action)
		m_escape_action->setShortcut(resolve("escape", "Escape"));
}

void main_window_t::shortcut_copy_original()
{
	if (!m_shortcuts_controller)
		return;

	m_shortcuts_controller->copy_original();
}

void main_window_t::shortcut_commit_status(status_t new_status)
{
	if (!m_shortcuts_controller)
		return;

	m_shortcuts_controller->commit_status(new_status);
}

void main_window_t::advance_to_next_row()
{
	if (m_editor_controller.current_row() < 0)
		return;

	commit_current_edit();

	const int row_count = m_table_model->rowCount();
	int next_row = -1;

	for (int i = m_editor_controller.current_row() + 1; i < row_count; ++i)
	{
		const auto * row_data = m_table_model->row_at(i);
		if (row_data && row_data->status != status_t::propagated)
		{
			next_row = i;
			break;
		}
	}

	if (next_row < 0)
	{
		next_row = m_editor_controller.current_row() + 1;
		if (next_row >= row_count)
			next_row = row_count - 1;
	}

	if (next_row < 0 || next_row == m_editor_controller.current_row())
		return;

	auto idx = m_table_model->index(next_row, 0);
	m_table_view->selectionModel()->setCurrentIndex(
	    idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	on_row_selected(next_row);

	auto cursor = m_editor_view->translation_editor()->textCursor();
	cursor.movePosition(QTextCursor::End);
	m_editor_view->translation_editor()->setTextCursor(cursor);
	m_editor_view->translation_editor()->setFocus();
}

void main_window_t::rebuild_annotations()
{
	std::vector<dict_source_t> sources;
	for (auto * dict_doc : m_session.all_dicts())
		sources.push_back({ &dict_doc->data(), dict_doc->path() });

	m_glossary.rebuild(sources);

	std::vector<std::string> loc_paths;
	for (const auto * entry : m_file_list.workspace_files())
	{
		if (entry->type != file_type_t::loc_file)
			continue;

		const auto extension = string_utils::to_lower(string_utils::extract_filename(entry->path));
		if (extension.find(".top") == std::string::npos && extension.find(".mrk") == std::string::npos)
			continue;

		loc_paths.push_back(entry->path);
	}

	m_inflection_store.rebuild(loc_paths, m_current_codepage);
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
		m_status_filter_view->set_filter_state(m_status_filter);
	}
	else
	{
		m_type_filter = {
			rec_type_t::cell, rec_type_t::dial, rec_type_t::info, rec_type_t::fnam, rec_type_t::text,
			rec_type_t::gmst, rec_type_t::desc, rec_type_t::rnam, rec_type_t::indx, rec_type_t::sctx,
		};
		m_status_filter.clear();
		m_type_filter_solo = false;
		m_filter_tree_view->set_active_types(m_type_filter);
		m_filter_tree_view->set_active_sub_types({});
		m_status_filter_view->set_filter_state(m_status_filter);
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
	if (!m_active_doc)
		return;

	if (m_active_doc->kind() == document_kind_t::yaml)
	{
		rebuild_table();
		return;
	}

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

void main_window_t::on_spell_lang_changed()
{
	const auto aff_path = m_settings.spell_aff_path();
	const auto dic_path = m_settings.spell_dic_path();

	if (aff_path.empty() || dic_path.empty())
	{
		m_hl_translation->set_spell_checker(nullptr);
		return;
	}

	if (!m_spell_checker.load(aff_path, dic_path))
		return;

	if (m_spell_check && m_spell_check->isChecked())
		m_hl_translation->set_spell_checker(&m_spell_checker);
}

void main_window_t::load_config()
{
	move(m_settings.window_x(), m_settings.window_y());
	resize(m_settings.window_width(), m_settings.window_height());

	if (m_settings.window_maximized())
		showMaximized();

	const int encoding_index = m_settings.encoding_index();
	if (encoding_index >= 0 && encoding_index < static_cast<int>(std::size(supported_codepages)))
		m_current_codepage = index_to_codepage(encoding_index);

	m_session.set_codepage(m_current_codepage);
	m_session.set_native_language(m_settings.native_language());
	m_byte_limit_validator.set_codepage(m_current_codepage);

	m_translation_tab->apply_provider_settings(m_settings);

	m_sidebar_toggle->setChecked(m_settings.sidebar_visible());
	m_bottom_panel_toggle->setChecked(m_settings.bottom_visible());
	m_sync_scroll_check->setChecked(m_settings.sync_scroll_enabled());

	const int highlight_mask = m_settings.highlight_kinds_mask();
	std::set<highlight_kind_t> enabled_kinds;
	if (highlight_mask & 0x1)
		enabled_kinds.insert(highlight_kind_t::hyperlink);

	if (highlight_mask & 0x2)
		enabled_kinds.insert(highlight_kind_t::inflection);

	if (highlight_mask & 0x4)
		enabled_kinds.insert(highlight_kind_t::glossary);

	m_editor_view->set_enabled_highlight_kinds(enabled_kinds);

	const float split_ratio = m_settings.split_ratio();
	if (split_ratio > 0.0f)
		m_editor_view->set_split_ratio(split_ratio);

	const int sidebar_width = m_settings.sidebar_width();
	const int total_width = width();
	m_central_splitter->setSizes({ sidebar_width, total_width - sidebar_width });

	const int info_height = m_settings.info_height();
	const int left_total = m_left_splitter->height();
	if (info_height > 0 && left_total > 0)
		m_left_splitter->setSizes({ left_total - info_height, info_height });
	else
		m_left_splitter->setStretchFactor(0, 2);

	const int bottom_height = m_settings.bottom_height();
	const int right_total = m_right_splitter->height();
	if (bottom_height > 0 && right_total > 0)
		m_right_splitter->setSizes({ right_total - bottom_height, bottom_height });
	else
		m_right_splitter->setStretchFactor(0, 2);

	std::vector<int> col_widths;
	for (int i = 0; i < 4; ++i)
		col_widths.push_back(m_settings.column_width(i));
	m_table_view->set_column_widths(col_widths);

	const auto workspace = resource_paths::workspace_dir();
	std::vector<std::string> roots = { workspace };
	for (const auto & saved_root : m_settings.workspace_roots())
	{
		if (!string_utils::paths_equal(saved_root, workspace))
			roots.push_back(saved_root);
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

	on_spell_lang_changed();

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
	m_settings.set_sync_scroll_enabled(m_sync_scroll_check->isChecked());

	m_settings.set_split_ratio(static_cast<float>(m_editor_view->get_split_ratio()));

	const auto left_sizes = m_left_splitter->sizes();
	if (left_sizes.size() >= 2)
		m_settings.set_info_height(left_sizes[1]);

	const auto right_sizes = m_right_splitter->sizes();
	if (right_sizes.size() >= 2)
		m_settings.set_bottom_height(right_sizes[1]);

	const auto central_sizes = m_central_splitter->sizes();
	if (central_sizes.size() >= 2)
		m_settings.set_sidebar_width(central_sizes[0]);

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
		    tr("Unsaved Changes"),
		    tr("You have unsaved dictionary changes. What would you like to do?"),
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

	save_config();
	QMainWindow::closeEvent(event);
}

bool main_window_t::is_row_example(int row) const
{
	const auto * row_data = m_table_model->row_at(row);
	if (!row_data)
		return false;

	const auto examples = m_settings.translation_examples();

	return translation_example_ops::contains_original(examples, row_data->old_text);
}

bool main_window_t::can_revert_row(int row) const
{
	const auto * row_data = m_table_model->row_at(row);
	if (!row_data)
		return false;

	return !m_edit_history.get_history(row_data->type, row_data->key_text).empty();
}

void main_window_t::on_toggle_example_requested(const QList<int> & rows)
{
	if (rows.isEmpty())
		return;

	const auto * first_row = m_table_model->row_at(rows.first());
	if (!first_row)
		return;

	auto examples = m_settings.translation_examples();
	const bool unmark = translation_example_ops::contains_original(examples, first_row->old_text);

	if (unmark)
	{
		for (const int row : rows)
		{
			const auto * row_data = m_table_model->row_at(row);
			if (!row_data)
				continue;

			examples = translation_example_ops::remove_by_original(examples, row_data->old_text);
		}
	}
	else
	{
		std::vector<translation_example_t> pairs;
		for (const int row : rows)
		{
			const auto * row_data = m_table_model->row_at(row);
			if (!row_data)
				continue;

			pairs.push_back({ row_data->old_text, row_data->new_text });
		}

		const auto before = examples.size();
		examples = translation_example_ops::add_capped(examples, pairs);

		if (examples.size() == before && !pairs.empty())
			statusBar()->showMessage(tr("Example limit reached (maximum %1)").arg(max_examples), 5000);
	}

	m_settings.set_translation_examples(examples);
	m_settings.sync();

	m_translation_tab->set_examples(examples);
}
