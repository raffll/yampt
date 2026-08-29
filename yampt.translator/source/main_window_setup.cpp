#include <resource_paths.hpp>
#include "dialog/dict_selection_dialog.hpp"
#include "dialog/first_run_dialog.hpp"
#include "dialog/spell_context_menu.hpp"
#include "highlighter/editor_highlighter.hpp"
#include "highlighter/glossary_highlighter.hpp"
#include "main_window.hpp"
#include "model/dict_document.hpp"
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
#include <utility/string_utils.hpp>
#include <algorithm>
#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

void main_window_t::setup_menu_bar()
{
	auto * file_menu = menuBar()->addMenu(tr("&File"));
	m_translator_file_menu = file_menu;

	m_add_folder_action = new QAction(tr("Add &Folder..."), this);
	m_add_folder_action->setToolTip(tr("Add a workspace folder"));
	file_menu->addAction(m_add_folder_action);

	m_import_archive_action = new QAction(tr("&Import Archive..."), this);
	m_import_archive_action->setToolTip(tr("Extract and import an archive into workspace"));
	file_menu->addAction(m_import_archive_action);

	file_menu->addSeparator();

	m_save_action = new QAction(tr("&Save"), this);
	m_save_action->setShortcut(QKeySequence("Ctrl+S"));
	m_save_action->setToolTip(tr("Save the active dictionary"));
	file_menu->addAction(m_save_action);

	m_save_all_action = new QAction(tr("Save A&ll"), this);
	m_save_all_action->setToolTip(tr("Save all modified dictionaries"));
	file_menu->addAction(m_save_all_action);

	file_menu->addSeparator();

	m_quit_action = new QAction(tr("&Quit"), this);
	m_quit_action->setShortcut(QKeySequence("Alt+F4"));
	m_quit_action->setToolTip(tr("Exit the application"));
	file_menu->addAction(m_quit_action);

	auto * view_menu = menuBar()->addMenu(tr("&View"));
	m_translator_view_menu = view_menu;

	m_sidebar_toggle = new QAction(tr("Toggle &Sidebar"), this);
	m_sidebar_toggle->setCheckable(true);
	m_sidebar_toggle->setChecked(true);
	m_sidebar_toggle->setToolTip(tr("Show or hide the sidebar panel"));
	view_menu->addAction(m_sidebar_toggle);

	m_bottom_panel_toggle = new QAction(tr("Toggle &Bottom Panel"), this);
	m_bottom_panel_toggle->setCheckable(true);
	m_bottom_panel_toggle->setChecked(true);
	m_bottom_panel_toggle->setToolTip(tr("Show or hide the editor panel"));
	view_menu->addAction(m_bottom_panel_toggle);

	view_menu->addSeparator();

	m_spell_check = new QAction(tr("&Spell Check"), this);
	m_spell_check->setCheckable(true);
	m_spell_check->setChecked(true);
	m_spell_check->setToolTip(tr("Toggle spell check underlines in translation"));
	view_menu->addAction(m_spell_check);

	m_grammar_check = new QAction(tr("&Grammar Check"), this);
	m_grammar_check->setCheckable(true);
	m_grammar_check->setChecked(true);
	m_grammar_check->setToolTip(tr("Toggle grammar check highlights in translation"));
	view_menu->addAction(m_grammar_check);

	m_whitespace_check = new QAction(tr("&Whitespace Markers"), this);
	m_whitespace_check->setCheckable(true);
	m_whitespace_check->setToolTip(tr("Show tabs and line endings in editor"));
	view_menu->addAction(m_whitespace_check);

	view_menu->addSeparator();

	m_sync_scroll_check = new QAction(tr("S&ync Scrolling"), this);
	m_sync_scroll_check->setCheckable(true);
	m_sync_scroll_check->setChecked(true);
	m_sync_scroll_check->setToolTip(tr("Sync scrolling between original and translation panes"));
	view_menu->addAction(m_sync_scroll_check);

	auto * tools_menu = menuBar()->addMenu(tr("&Tools"));
	auto * merge_action = tools_menu->addAction(tr("&Merge Dictionaries..."));
	merge_action->setToolTip(tr("Merge loaded dictionaries into one"));
	connect(
	    merge_action,
	    &QAction::triggered,
	    this,
	    [this]()
	{
		if (m_dict_ops_controller)
			m_dict_ops_controller->on_merge();
	});

	tools_menu->addSeparator();
	m_settings_action = tools_menu->addAction(tr("&Preferences..."));
	m_settings_action->setShortcut(QKeySequence("Ctrl+,"));
	m_settings_action->setToolTip(tr("Open application settings"));
	connect(m_settings_action, &QAction::triggered, this, &main_window_t::on_open_settings);
}

void main_window_t::setup_toolbar()
{
	m_toolbar = addToolBar("Main");
	m_toolbar->setMovable(false);

	setContextMenuPolicy(Qt::PreventContextMenu);

	m_search_field = new QLineEdit(this);
	m_search_field->setPlaceholderText(tr("Filter by..."));
	m_search_field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_toolbar->addWidget(m_search_field);

	m_case_sensitive_check = new QToolButton(this);
	m_case_sensitive_check->setText(tr("Aa"));
	m_case_sensitive_check->setCheckable(true);
	m_toolbar->addWidget(m_case_sensitive_check);

	m_regex_check = new QToolButton(this);
	m_regex_check->setText(tr(".*"));
	m_regex_check->setCheckable(true);
	m_toolbar->addWidget(m_regex_check);

	m_search_col_key = new QToolButton(this);
	m_search_col_key->setText(tr("Key"));
	m_search_col_key->setCheckable(true);
	m_search_col_key->setChecked(true);
	m_toolbar->addWidget(m_search_col_key);

	m_search_col_original = new QToolButton(this);
	m_search_col_original->setText(tr("Original"));
	m_search_col_original->setCheckable(true);
	m_search_col_original->setChecked(true);
	m_toolbar->addWidget(m_search_col_original);

	m_search_col_translation = new QToolButton(this);
	m_search_col_translation->setText(tr("Translation"));
	m_search_col_translation->setCheckable(true);
	m_search_col_translation->setChecked(true);
	m_toolbar->addWidget(m_search_col_translation);

	m_search_field->setToolTip(tr("Search across entries"));
	m_case_sensitive_check->setToolTip(tr("Case-sensitive search"));
	m_regex_check->setToolTip(tr("Regular expression search"));
	m_search_col_key->setToolTip(tr("Search in key column"));
	m_search_col_original->setToolTip(tr("Search in original column"));
	m_search_col_translation->setToolTip(tr("Search in translation column"));

	m_escape_action = new QAction(this);
	m_escape_action->setShortcut(QKeySequence("Escape"));
	addAction(m_escape_action);
}

void main_window_t::setup_replace_toolbar()
{
	m_replace_toolbar = new QWidget(this);

	auto * layout = new QVBoxLayout(m_replace_toolbar);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	m_find_field = new QLineEdit(this);
	m_find_field->setPlaceholderText(tr("Search in translations..."));
	m_find_field->setToolTip(tr("Text to find in the translation column"));
	layout->addWidget(m_find_field);

	m_replace_field = new QLineEdit(this);
	m_replace_field->setPlaceholderText(tr("Replace with..."));
	m_replace_field->setToolTip(tr("Replacement text"));
	layout->addWidget(m_replace_field);

	auto * options_row = new QHBoxLayout;
	options_row->setSpacing(4);

	m_replace_case_check = new QPushButton(tr("Aa"), this);
	m_replace_case_check->setCheckable(true);
	m_replace_case_check->setToolTip(tr("Case-sensitive find/replace"));
	options_row->addWidget(m_replace_case_check);

	m_replace_regex_check = new QPushButton(tr(".*"), this);
	m_replace_regex_check->setCheckable(true);
	m_replace_regex_check->setToolTip(tr("Regular expression find/replace"));
	options_row->addWidget(m_replace_regex_check);

	options_row->addStretch();
	layout->addLayout(options_row);

	auto * buttons_row = new QHBoxLayout;
	buttons_row->setSpacing(4);

	m_replace_one_btn = new QPushButton(tr("Replace"), this);
	m_replace_one_btn->setToolTip(tr("Replace in the selected entry and advance to next"));
	buttons_row->addWidget(m_replace_one_btn);

	m_replace_all_btn = new QPushButton(tr("Replace All"), this);
	m_replace_all_btn->setToolTip(tr("Replace in all currently visible entries"));
	buttons_row->addWidget(m_replace_all_btn);

	buttons_row->addStretch();
	layout->addLayout(buttons_row);

	layout->addStretch();
}

void main_window_t::setup_central_widget()
{
	auto * central_widget = new QWidget(this);
	auto * central_layout = new QVBoxLayout(central_widget);
	central_layout->setContentsMargins(0, 0, 0, 0);
	central_layout->setSpacing(4);

	setup_replace_toolbar();

	m_filter_tree_view = new filter_tree_view_t(this);
	m_status_filter_view = new status_filter_view_t(this);

	m_central_splitter = new QSplitter(Qt::Horizontal, central_widget);
	central_layout->addWidget(m_central_splitter, 1);

	setCentralWidget(central_widget);
}

void main_window_t::setup_sidebar()
{
	m_left_splitter = new QSplitter(Qt::Vertical, m_central_splitter);

	m_left_tabs = new QTabWidget(m_left_splitter);
	m_sidebar = new sidebar_view_t(m_left_tabs);
	m_left_tabs->addTab(m_sidebar, tr("Files"));
	m_left_tabs->addTab(m_filter_tree_view, tr("Filters"));
	m_left_tabs->addTab(m_status_filter_view, tr("Statuses"));
	m_left_tabs->addTab(m_replace_toolbar, tr("Find/Replace"));
	m_left_splitter->addWidget(m_left_tabs);

	m_info_tabs = new QTabWidget(m_left_splitter);
	m_annotations_view = new annotations_view_t(m_info_tabs);
	m_history_view = new history_view_t(m_info_tabs);
	m_translation_tab = new translation_suggestion_view_t(m_info_tabs);
	m_translation_tab->set_models_dir(resource_paths::models_dir());
	m_translation_tab->set_providers_dir(resource_paths::providers_dir());
	m_translation_tab->set_glossary_fn([this](const std::string & text) { return m_glossary.apply_glossary(text); });
	m_translation_tab->set_settings_store(m_settings);
	m_info_tabs->addTab(m_annotations_view, tr("Annotations"));
	m_info_tabs->addTab(m_translation_tab, tr("Auto Translate"));
	m_info_tabs->addTab(m_history_view, tr("History"));
	m_left_splitter->addWidget(m_info_tabs);
}

void main_window_t::setup_editor_panel()
{
	m_right_splitter = new QSplitter(Qt::Vertical, m_central_splitter);

	auto * right_top_widget = new QWidget(m_right_splitter);
	auto * right_top_layout = new QVBoxLayout(right_top_widget);
	right_top_layout->setContentsMargins(0, 0, 0, 0);
	right_top_layout->setSpacing(0);

	m_record_tabs = new QTabWidget(right_top_widget);
	m_table_model = new record_table_model_t(this);
	m_table_view = new record_table_view_t(m_record_tabs);
	m_table_view->setModel(m_table_model);
	m_book_preview_view = new book_preview_view_t(m_record_tabs);
	m_log_view = new log_view_t(m_record_tabs);
	m_record_tabs->addTab(m_table_view, tr("Records"));
	m_record_tabs->addTab(m_book_preview_view, tr("Preview"));
	m_record_tabs->addTab(m_log_view, tr("Log"));
	right_top_layout->addWidget(m_record_tabs, 1);

	m_right_splitter->addWidget(right_top_widget);

	m_editor_view = new editor_view_t(m_right_splitter);
	m_right_splitter->addWidget(m_editor_view);

	m_central_splitter->addWidget(m_left_splitter);
	m_central_splitter->addWidget(m_right_splitter);
	m_central_splitter->setSizes({ 250, 1030 });

	m_hl_original = new editor_highlighter_t(m_editor_view->original_view()->document());
	m_hl_adapted = new editor_highlighter_t(m_editor_view->details_view()->document());
	m_hl_translation = new editor_highlighter_t(m_editor_view->translation_editor()->document());
	m_hl_translation->set_translation_mode(true);

	m_spell_menu = new spell_context_menu_t(&m_spell_checker, m_hl_translation);
}

void main_window_t::setup_status_bar()
{
	m_active_file_label = new QLabel(this);
	statusBar()->addWidget(m_active_file_label, 1);

	m_progress_label = new QLabel(this);
	statusBar()->addPermanentWidget(m_progress_label);

	m_validation_view = new validation_view_t(this);
	statusBar()->addPermanentWidget(m_validation_view);
}

void main_window_t::setup_table_display()
{
	m_workspace_watcher = new workspace_watcher_t(this);

	m_table_display = std::make_unique<table_view_t>(
	    *m_filter_tree_view,
	    *m_status_filter_view,
	    *m_table_model,
	    *m_progress_label,
	    *m_active_file_label,
	    *m_search_field,
	    *m_case_sensitive_check,
	    *m_regex_check,
	    *m_search_col_key,
	    *m_search_col_original,
	    *m_search_col_translation);

	if (!m_find_replace)
		m_find_replace = new find_replace_t(*m_table_model, m_active_doc, m_edit_history);
}

void main_window_t::connect_menu_signals()
{
	connect(m_save_action, &QAction::triggered, this, &main_window_t::on_save);
	connect(m_save_all_action, &QAction::triggered, this, &main_window_t::on_save_all);

	connect(m_quit_action, &QAction::triggered, this, &QWidget::close);
	connect(m_escape_action, &QAction::triggered, this, &main_window_t::on_escape);

	connect(m_sidebar_toggle, &QAction::toggled, m_left_splitter, &QWidget::setVisible);
	connect(m_bottom_panel_toggle, &QAction::toggled, m_editor_view, &QWidget::setVisible);
	connect(m_whitespace_check, &QAction::toggled, this, &main_window_t::on_whitespace_toggled);

	connect(
	    m_sync_scroll_check,
	    &QAction::toggled,
	    this,
	    [this](bool checked)
	{
		m_editor_view->set_scroll_sync(checked);
		m_book_preview_view->set_scroll_sync(checked);
	});

	connect(
	    m_spell_check,
	    &QAction::toggled,
	    this,
	    [this](bool checked)
	{
		if (checked)
			m_hl_translation->set_spell_checker(&m_spell_checker);
		else
			m_hl_translation->set_spell_checker(nullptr);
	});

	connect(
	    m_grammar_check,
	    &QAction::toggled,
	    this,
	    [this]()
	{
		if (m_editor_controller.current_row() < 0)
			return;

		const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
		auto type = row_data ? row_data->type : rec_type_t::info;
		m_extra_sel_translation.grammar = m_grammar_check->isChecked()
		                                      ? m_grammar_checker.check(m_editor_view->translation_editor(), type)
		                                      : QList<QTextEdit::ExtraSelection> {};
		highlight_applier_t::apply(m_editor_view->translation_editor(), m_extra_sel_translation);
	});

	connect(
	    m_add_folder_action,
	    &QAction::triggered,
	    this,
	    [this]()
	{
		const auto folder = QFileDialog::getExistingDirectory(this, tr("Add Folder"));
		if (folder.isEmpty())
			return;

		const auto path = folder.toStdString();
		auto roots = m_file_list.get_roots();
		if (std::find(roots.begin(), roots.end(), path) != roots.end())
			return;

		roots.push_back(path);
		m_file_list.scan_roots(roots);
		scan_workspace();
		save_config();
		update_watcher_roots();
	});

	connect(
	    m_import_archive_action,
	    &QAction::triggered,
	    this,
	    [this]()
	{
		const auto archive_path =
		    QFileDialog::getOpenFileName(this, tr("Import Archive"), "", tr("Archives (*.zip *.rar)"));

		if (archive_path.isEmpty())
			return;

		const auto app_dir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
		const QString sevenzip = app_dir + "/7za.exe";
		if (!QFile::exists(sevenzip))
		{
			QMessageBox::critical(this, tr("Error"), tr("7za.exe not found next to the application"));
			return;
		}
#else
		const QString sevenzip = QStandardPaths::findExecutable("7z");
		if (sevenzip.isEmpty())
		{
			QMessageBox::critical(this, tr("Error"), tr("7z not found in PATH"));
			return;
		}
#endif

		const auto archive_name = QFileInfo(archive_path).completeBaseName();
		const auto target_dir =
		    QString::fromStdString(resource_paths::workspace_dir()) + archive_name;
		QDir().mkpath(target_dir);

		QProcess proc;
		proc.start(sevenzip, { "x", archive_path, "-o" + target_dir, "-y" });
		proc.waitForFinished(60000);

		if (proc.exitCode() != 0)
		{
			QMessageBox::critical(
			    this, tr("Extraction Error"), tr("Failed to extract archive:\n") + proc.readAllStandardError());
			return;
		}

		scan_workspace();
		update_watcher_roots();
	});

	connect(
	    m_workspace_watcher,
	    &workspace_watcher_t::workspace_changed,
	    this,
	    [this]()
	{
		scan_workspace();

		if (m_active_doc && !QFile::exists(QString::fromStdString(m_active_doc->path())))
		{
			const auto path = m_active_doc->path();
			switch_document(nullptr);
			m_session.close(path);
			m_filter_states.erase(path);
			rebuild_annotations();
			m_last_annotation_version = m_session.dict_version();
		}
	});

	connect(
	    m_replace_all_btn,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		if (!m_find_replace)
			return;

		const auto & query = m_find_field->text().toStdString();
		const auto & replacement = m_replace_field->text().toStdString();
		const bool case_sensitive = m_replace_case_check->isChecked();
		const bool regex_mode = m_replace_regex_check->isChecked();

		auto result = m_find_replace->replace_all(query, replacement, case_sensitive, regex_mode);

		if (result.count > 0)
		{
			set_unsaved_changes(true);
			rebuild_table();
			if (m_editor_controller.current_row() >= 0)
				load_record(m_editor_controller.current_row());
		}

		statusBar()->showMessage(tr("Replaced in %1 entries").arg(result.count), 5000);
	});

	connect(
	    m_replace_one_btn,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		if (!m_find_replace)
			return;

		const int current_row = m_editor_controller.current_row();
		if (current_row < 0)
			return;

		const auto & query = m_find_field->text().toStdString();
		const auto & replacement = m_replace_field->text().toStdString();
		const bool case_sensitive = m_replace_case_check->isChecked();
		const bool regex_mode = m_replace_regex_check->isChecked();

		if (!m_find_replace->replace_one(current_row, query, replacement, case_sensitive, regex_mode))
			return;

		set_unsaved_changes(true);
		rebuild_table();
		advance_to_next_row();
	});

}

void main_window_t::connect_sidebar_signals()
{
	connect(m_sidebar, &sidebar_view_t::item_clicked, this, &main_window_t::on_item_clicked);
	connect(m_sidebar, &sidebar_view_t::operation_requested, this, &main_window_t::on_operation_requested);
	connect(m_sidebar, &sidebar_view_t::save_requested, this, &main_window_t::on_save_requested);
	connect(m_sidebar, &sidebar_view_t::unload_requested, this, &main_window_t::on_unload_requested);
	connect(m_sidebar, &sidebar_view_t::delete_requested, this, &main_window_t::on_delete_requested);

	connect(
	    m_sidebar,
	    &sidebar_view_t::export_native_requested,
	    this,
	    [this](const std::string & path) { m_sidebar_controller->on_export_native_requested(path); });

	connect(
	    m_sidebar,
	    &sidebar_view_t::generate_loc_requested,
	    this,
	    [this](const std::string & path) { m_sidebar_controller->on_generate_loc_requested(path); });

	connect(
	    m_sidebar,
	    &sidebar_view_t::export_eet_requested,
	    this,
	    [this](const std::string & path) { m_sidebar_controller->on_export_eet_requested(path); });

	connect(
	    m_sidebar,
	    &sidebar_view_t::remove_folder_requested,
	    this,
	    [this](const std::string & root_path) { m_sidebar_controller->on_remove_folder_requested(root_path); });

	connect(
	    m_sidebar,
	    &sidebar_view_t::delete_folder_requested,
	    this,
	    [this](const std::string & folder_path) { m_sidebar_controller->on_delete_folder_requested(folder_path); });

	connect(
	    m_history_view,
	    &history_view_t::revert_requested,
	    this,
	    [this](size_t history_index)
	{
		if (m_editor_controller.current_row() < 0)
			return;

		const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
		if (!row_data)
			return;

		auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
		if (!dict_doc)
			return;

		auto & data = dict_doc->data_mut();
		auto type_it = data.find(row_data->type);
		if (type_it == data.end())
			return;

		auto * record = type_it->second.find(row_data->key_text);
		if (!record)
			return;

		auto index_it = type_it->second.index.find(row_data->key_text);
		if (index_it == type_it->second.index.end())
			return;

		const auto history = m_edit_history.get_history(row_data->type, row_data->key_text);
		if (history_index >= history.size())
			return;

		std::string current_value = record->new_text;
		const auto current_status = record->status;
		record->new_text = history[history_index].value;
		record->status = history[history_index].status;
		dict_doc->set_dirty(true);
		dict_doc->modified_records_insert(row_data->type, index_it->second);
		set_unsaved_changes(true);

		m_edit_history.record_change(
		    row_data->type, row_data->key_text, current_value, record->new_text, current_status);

		m_table_model->update_row(m_editor_controller.current_row(), record->new_text, record->status);
		load_record(m_editor_controller.current_row());
	});
}

void main_window_t::connect_editor_signals()
{
	connect(m_table_view, &record_table_view_t::row_selected, this, &main_window_t::on_row_selected);

	m_table_view->set_example_state_fn([this](int row) { return is_row_example(row); });

	m_table_view->set_example_count_fn([this]() { return static_cast<int>(m_settings.translation_examples().size()); });

	m_table_view->set_can_revert_fn([this](int row) { return can_revert_row(row); });

	connect(
	    m_table_view,
	    &record_table_view_t::toggle_example_requested,
	    this,
	    &main_window_t::on_toggle_example_requested);

	connect(
	    m_table_model,
	    &record_table_model_t::inline_edit_committed,
	    this,
	    [this](int row, const std::string & new_text)
	{
		if (!m_active_doc)
			return;

		const auto * row_data = m_table_model->row_at(row);
		if (!row_data)
			return;

		m_edit_history.record_change(
		    row_data->type, row_data->key_text, row_data->new_text, new_text, row_data->status);

		const auto result = m_active_doc->commit(*row_data, new_text, status_t::in_progress);
		if (!result.success)
			return;

		m_table_model->update_row(row, result.new_text, result.status);
		set_unsaved_changes(m_active_doc->is_dirty());
		update_sidebar_item(m_active_doc->path());
		update_status_counts();

		int next_row = row + 1;
		if (next_row < m_table_model->rowCount())
		{
			auto idx = m_table_model->index(next_row, 0);
			m_table_view->selectionModel()->setCurrentIndex(
			    idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
			on_row_selected(next_row);
		}
	});

	connect(
	    m_table_view,
	    &record_table_view_t::delete_entry_requested,
	    this,
	    [this]()
	{
		if (m_editor_controller.current_row() < 0)
			return;

		if (!m_active_doc)
			return;

		const auto * row_data = m_table_model->row_at(m_editor_controller.current_row());
		if (!row_data)
			return;

		const auto result = m_active_doc->reset_to_original(*row_data);
		if (!result.success)
			return;

		m_table_model->update_row(m_editor_controller.current_row(), result.new_text, result.status);
		set_unsaved_changes(true);
		update_status_counts();
		load_record(m_editor_controller.current_row());
	});

	connect(
	    m_table_view,
	    &record_table_view_t::batch_status_change_requested,
	    this,
	    [this](const QList<int> & rows, status_t new_status)
	{
		if (!m_active_doc)
			return;

		for (int row : rows)
		{
			const auto * row_data = m_table_model->row_at(row);
			if (!row_data)
				continue;

			const auto result = m_active_doc->commit_status(*row_data, new_status);
			if (result.success)
				m_table_model->update_row(row, result.new_text, result.status);
		}

		set_unsaved_changes(m_active_doc->is_dirty());
		update_status_counts();
	});

	connect(
	    m_table_view,
	    &record_table_view_t::batch_revert_requested,
	    this,
	    [this](const QList<int> & rows)
	{
		if (!m_active_doc)
			return;

		auto * dict_doc = dynamic_cast<dict_document_t *>(m_active_doc);
		if (!dict_doc)
			return;

		int reverted_count = 0;

		for (int row : rows)
		{
			const auto * row_data = m_table_model->row_at(row);
			if (!row_data)
				continue;

			const auto history = m_edit_history.get_history(row_data->type, row_data->key_text);
			if (history.empty())
				continue;

			const auto revert = m_edit_history.revert(row_data->type, row_data->key_text, history.size() - 1);
			if (!revert.success)
				continue;

			auto & data = dict_doc->data_mut();
			auto type_it = data.find(row_data->type);
			if (type_it == data.end() || row_data->record_index >= type_it->second.records.size())
				continue;

			auto & entry = type_it->second.records[row_data->record_index];
			entry.new_text = revert.reverted_text;
			entry.status = revert.reverted_status;
			dict_doc->modified_records_insert(row_data->type, row_data->record_index);

			m_table_model->update_row(row, revert.reverted_text, revert.reverted_status);
			++reverted_count;
		}

		if (reverted_count > 0)
		{
			dict_doc->set_dirty(true);
			set_unsaved_changes(true);
			update_status_counts();

			if (m_editor_controller.current_row() >= 0)
				load_record(m_editor_controller.current_row());
		}

		statusBar()->showMessage(tr("Reverted %1 entries").arg(reverted_count), 5000);
	});

	connect(m_editor_view, &editor_view_t::text_changed, this, &main_window_t::on_translation_changed);

	connect(m_editor_view, &editor_view_t::apply_clicked, this, [this]() { advance_to_next_row(); });

	connect(
	    m_editor_view->translation_editor(),
	    &translation_edit_view_t::navigate_next,
	    this,
	    [this]() { advance_to_next_row(); });

	connect(
	    m_editor_view->translation_editor(),
	    &translation_edit_view_t::navigate_prev,
	    this,
	    [this]()
	{
		if (m_editor_controller.current_row() <= 0)
			return;

		commit_current_edit();

		int prev_row = m_editor_controller.current_row() - 1;
		auto idx = m_table_model->index(prev_row, 0);
		m_table_view->selectionModel()->setCurrentIndex(
		    idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
		on_row_selected(prev_row);
		auto cursor = m_editor_view->translation_editor()->textCursor();
		cursor.movePosition(QTextCursor::End);
		m_editor_view->translation_editor()->setTextCursor(cursor);
		m_editor_view->translation_editor()->setFocus();
	});

	connect(
	    m_translation_tab,
	    &translation_suggestion_view_t::translate_all_requested,
	    this,
	    [this]()
	{
		if (!m_active_doc)
		{
			m_translation_tab->append_log("[error] no document loaded\n");
			return;
		}

		auto * provider = m_translation_tab->active_provider();
		if (!provider || !provider->is_available())
		{
			m_translation_tab->append_log("[error] translation provider not available\n");
			return;
		}

		const int current_row = m_editor_controller.current_row();
		if (current_row < 0)
		{
			m_translation_tab->append_log("[error] no row selected\n");
			return;
		}

		const auto * row_data = m_table_model->row_at(current_row);
		if (!row_data)
			return;

		if (row_data->status != status_t::untranslated)
		{
			m_translation_tab->append_log("[info] only untranslated entries can be translated\n");
			return;
		}

		m_translation_tab->set_source_text(row_data->old_text);
		m_translation_tab->set_target_language(m_settings.native_language());

		if (m_editor_view->has_script_template())
		{
			const auto lines = m_editor_view->original_view()->toPlainText().split('\n');
			std::vector<std::string> prepared_lines;

			for (const auto & line : lines)
			{
				const auto source = line.toStdString();
				if (source.empty())
					prepared_lines.push_back(std::string());
				else
					prepared_lines.push_back(m_glossary.apply_glossary(source));
			}

			m_translation_tab->request_translation_lines(prepared_lines);
		}
		else
		{
			const auto prepared = m_glossary.apply_glossary(row_data->old_text);
			m_translation_tab->request_translation(prepared);
		}
	});

	connect(
	    m_translation_tab,
	    &translation_suggestion_view_t::translation_committed,
	    this,
	    [this](const std::string & result_text)
	{
		m_editor_view->translation_editor()->setPlainText(QString::fromStdString(result_text));
		m_editor_controller.set_pending_status(status_t::model);
		advance_to_next_row();
	});

	connect(
	    m_translation_tab,
	    &translation_suggestion_view_t::translation_lines_committed,
	    this,
	    [this](const std::vector<std::string> & result_lines)
	{
		QStringList joined;
		for (const auto & line : result_lines)
			joined.append(QString::fromStdString(line));

		m_editor_view->translation_editor()->setPlainText(joined.join('\n'));
		m_editor_controller.set_pending_status(status_t::model);
		advance_to_next_row();
	});

	connect(
	    m_translation_tab,
	    &translation_suggestion_view_t::translation_failed,
	    this,
	    [this](const std::string & error_message)
	{ m_translation_tab->append_log("[error] " + error_message + "\n"); });
}

void main_window_t::connect_search_signals()
{
	connect(m_search_field, &QLineEdit::textChanged, this, &main_window_t::on_search_changed);
	connect(m_case_sensitive_check, &QToolButton::toggled, this, [this]() { on_case_sensitive_changed(0); });
	connect(m_regex_check, &QToolButton::toggled, this, [this]() { on_search_changed(m_search_query); });

	auto on_search_col_changed = [this]() { on_search_changed(m_search_query); };
	connect(m_search_col_key, &QToolButton::toggled, this, on_search_col_changed);
	connect(m_search_col_original, &QToolButton::toggled, this, on_search_col_changed);
	connect(m_search_col_translation, &QToolButton::toggled, this, on_search_col_changed);

	connect(m_filter_tree_view, &filter_tree_view_t::filters_changed, this, &main_window_t::on_filters_changed);
	connect(
	    m_status_filter_view, &status_filter_view_t::filters_changed, this, &main_window_t::on_status_filters_changed);
}
