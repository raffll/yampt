#include "main_window.hpp"
#include "highlight/annotation_highlighter.hpp"
#include "highlight/composite_highlighter.hpp"
#include "translate/ctranslate2_translator.hpp"
#include "model/dict_document.hpp"
#include "utility/display_name.hpp"
#include "dialog/dict_selection_dialog.hpp"
#include "view/editor_view.hpp"
#include "view/filter_tree_view.hpp"
#include "dialog/find_replace_dialog.hpp"
#include "dialog/first_run_dialog.hpp"
#include "view/log_view.hpp"
#include "view/record_table_view.hpp"
#include "view/sidebar_view.hpp"
#include "dialog/spell_context_menu.hpp"
#include "view/status_filter_view.hpp"
#include "view/translation_suggestion_view.hpp"
#include "view/validation_view.hpp"
#include "view/annotations_view.hpp"
#include "view/book_preview_view.hpp"
#include "view/history_view.hpp"
#include "model/yaml_document.hpp"
#include <utility/string_utils.hpp>

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <algorithm>

void main_window_t::setup_menu_bar()
{
	auto * file_menu = menuBar()->addMenu("&File");
	translator_file_menu_ = file_menu;

	add_folder_action_ = new QAction("Add &Folder...", this);
	file_menu->addAction(add_folder_action_);

	import_archive_action_ = new QAction("&Import Archive...", this);
	file_menu->addAction(import_archive_action_);

	file_menu->addSeparator();

	save_action_ = new QAction("&Save", this);
	save_action_->setShortcut(QKeySequence("Ctrl+S"));
	file_menu->addAction(save_action_);

	save_all_action_ = new QAction("Save A&ll", this);
	file_menu->addAction(save_all_action_);

	file_menu->addSeparator();

	quit_action_ = new QAction("&Quit", this);
	quit_action_->setShortcut(QKeySequence("Alt+F4"));
	file_menu->addAction(quit_action_);

	auto * view_menu = menuBar()->addMenu("&View");
	translator_view_menu_ = view_menu;

	sidebar_toggle_ = new QAction("Toggle &Sidebar", this);
	sidebar_toggle_->setCheckable(true);
	sidebar_toggle_->setChecked(true);
	view_menu->addAction(sidebar_toggle_);

	bottom_panel_toggle_ = new QAction("Toggle &Bottom Panel", this);
	bottom_panel_toggle_->setCheckable(true);
	bottom_panel_toggle_->setChecked(true);
	view_menu->addAction(bottom_panel_toggle_);

	view_menu->addSeparator();

	grammar_check_ = new QAction("&Grammar Check", this);
	grammar_check_->setCheckable(true);
	grammar_check_->setChecked(true);
	view_menu->addAction(grammar_check_);

	whitespace_check_ = new QAction("&Whitespace Markers", this);
	whitespace_check_->setCheckable(true);
	view_menu->addAction(whitespace_check_);

	view_menu->addSeparator();

	auto * encoding_menu = view_menu->addMenu("&Encoding");
	encoding_group_ = new QActionGroup(this);
	const QStringList encodings = { "Windows-1250", "Windows-1251", "Windows-1252" };
	for (int i = 0; i < encodings.size(); ++i)
	{
		auto * act = encoding_menu->addAction(encodings[i]);
		act->setCheckable(true);
		act->setData(i);
		encoding_group_->addAction(act);
	}
	encoding_group_->actions().last()->setChecked(true);

	spelling_menu_ = view_menu->addMenu("&Spelling");
	spelling_group_ = new QActionGroup(this);
	auto * none_act = spelling_menu_->addAction("None");
	none_act->setCheckable(true);
	none_act->setChecked(true);
	none_act->setData(0);
	spelling_group_->addAction(none_act);
}

void main_window_t::setup_toolbar()
{
	toolbar_ = new QToolBar(this);
	toolbar_->setMovable(false);

	search_label_ = new QLabel("Filter by:", this);
	search_label_->setStyleSheet("QLabel:disabled { color: rgb(180,180,180); }");
	toolbar_->addWidget(search_label_);

	search_field_ = new QLineEdit(this);
	search_field_->setPlaceholderText("Search...");
	toolbar_->addWidget(search_field_);

	static const QString toggle_style = "QPushButton { border: 1px solid #bbb; border-radius: 2px; padding: "
	                                    "2px 6px; background: #f0f0f0; }"
	                                    "QPushButton:checked { background: #cde; border-color: #89a; }"
	                                    "QPushButton:disabled { color: rgb(180,180,180); }";

	case_sensitive_check_ = new QPushButton("Aa", this);
	case_sensitive_check_->setCheckable(true);
	case_sensitive_check_->setFlat(false);
	case_sensitive_check_->setStyleSheet(toggle_style);
	toolbar_->addWidget(case_sensitive_check_);

	regex_check_ = new QPushButton(".*", this);
	regex_check_->setCheckable(true);
	regex_check_->setStyleSheet(toggle_style);
	toolbar_->addWidget(regex_check_);

	search_col_key_ = new QPushButton("Key", this);
	search_col_key_->setCheckable(true);
	search_col_key_->setChecked(true);
	search_col_key_->setStyleSheet(toggle_style);
	toolbar_->addWidget(search_col_key_);

	search_col_original_ = new QPushButton("Original", this);
	search_col_original_->setCheckable(true);
	search_col_original_->setChecked(true);
	search_col_original_->setStyleSheet(toggle_style);
	toolbar_->addWidget(search_col_original_);

	search_col_translation_ = new QPushButton("Translation", this);
	search_col_translation_->setCheckable(true);
	search_col_translation_->setChecked(true);
	search_col_translation_->setStyleSheet(toggle_style);
	toolbar_->addWidget(search_col_translation_);

	search_field_->setToolTip("Search across entries");
	case_sensitive_check_->setToolTip("Case-sensitive search");
	regex_check_->setToolTip("Regular expression search");
	search_col_key_->setToolTip("Search in key column");
	search_col_original_->setToolTip("Search in original column");
	search_col_translation_->setToolTip("Search in translation column");

	find_action_ = new QAction(this);
	find_action_->setShortcut(QKeySequence("Ctrl+F"));
	addAction(find_action_);

	escape_action_ = new QAction(this);
	escape_action_->setShortcut(QKeySequence("Escape"));
	addAction(escape_action_);
}

void main_window_t::setup_central_widget()
{
	auto * central_widget = new QWidget(this);
	auto * central_layout = new QVBoxLayout(central_widget);
	central_layout->setContentsMargins(0, 0, 0, 0);
	central_layout->setSpacing(4);

	central_layout->addWidget(toolbar_);

	filter_tree_view_ = new filter_tree_view_t(this);
	status_filter_view_ = new status_filter_view_t(central_widget);
	central_layout->addWidget(status_filter_view_);

	central_splitter_ = new QSplitter(Qt::Horizontal, central_widget);
	central_layout->addWidget(central_splitter_, 1);

	setCentralWidget(central_widget);
}

void main_window_t::setup_sidebar()
{
	left_splitter_ = new QSplitter(Qt::Vertical, central_splitter_);

	left_tabs_ = new QTabWidget(left_splitter_);
	sidebar_ = new sidebar_view_t(left_tabs_);
	left_tabs_->addTab(sidebar_, "Files");
	left_tabs_->addTab(filter_tree_view_, "Filters");
	left_splitter_->addWidget(left_tabs_);

	info_tabs_ = new QTabWidget(left_splitter_);
	annotations_view_ = new annotations_view_t(info_tabs_);
	history_view_ = new history_view_t(info_tabs_);
	translation_tab_ = new translation_suggestion_view_t(info_tabs_);
	translation_tab_->set_models_dir((QCoreApplication::applicationDirPath() + "/models").toStdString());
	find_replace_dialog_ = new find_replace_dialog_t(this);
	find_replace_dialog_->setVisible(false);
	info_tabs_->addTab(annotations_view_, "Annotations");
	info_tabs_->addTab(history_view_, "History");
	info_tabs_->addTab(translation_tab_, "Translate");
	left_splitter_->addWidget(info_tabs_);
}

void main_window_t::setup_editor_panel()
{
	right_splitter_ = new QSplitter(Qt::Vertical, central_splitter_);

	auto * right_top_widget = new QWidget(right_splitter_);
	auto * right_top_layout = new QVBoxLayout(right_top_widget);
	right_top_layout->setContentsMargins(0, 0, 0, 0);
	right_top_layout->setSpacing(0);

	record_tabs_ = new QTabWidget(right_top_widget);
	table_model_ = new record_table_model_t(this);
	table_view_ = new record_table_view_t(record_tabs_);
	table_view_->setModel(table_model_);
	book_preview_view_ = new book_preview_view_t(record_tabs_);
	log_view_ = new log_view_t(record_tabs_);
	record_tabs_->addTab(table_view_, "Records");
	record_tabs_->addTab(book_preview_view_, "Book Preview");
	record_tabs_->addTab(log_view_, "Log");
	right_top_layout->addWidget(record_tabs_, 1);

	right_splitter_->addWidget(right_top_widget);

	editor_view_ = new editor_view_t(right_splitter_);
	right_splitter_->addWidget(editor_view_);

	central_splitter_->addWidget(left_splitter_);
	central_splitter_->addWidget(right_splitter_);
	central_splitter_->setSizes({ 250, 1030 });

	hl_original_ = new composite_highlighter_t(editor_view_->original_view()->document());
	hl_adapted_ = new composite_highlighter_t(editor_view_->details_view()->document());
	hl_translation_ = new composite_highlighter_t(editor_view_->translation_editor()->document());
	hl_translation_->set_translation_mode(true);

	spell_menu_ = new spell_context_menu_t(&spell_checker_, hl_translation_);
}

void main_window_t::setup_status_bar()
{
	active_file_label_ = new QLabel(this);
	statusBar()->addWidget(active_file_label_, 1);

	progress_label_ = new QLabel(this);
	statusBar()->addPermanentWidget(progress_label_);

	validation_view_ = new validation_view_t(this);
	statusBar()->addPermanentWidget(validation_view_);
}

void main_window_t::setup_table_display()
{
	fs_watcher_ = new QFileSystemWatcher(this);
	rescan_timer_ = new QTimer(this);
	rescan_timer_->setSingleShot(true);
	rescan_timer_->setInterval(200);

	scan_spell_dictionaries();

	table_display_ = std::make_unique<table_view_t>(
	    *filter_tree_view_,
	    *status_filter_view_,
	    *table_model_,
	    *progress_label_,
	    *active_file_label_,
	    *search_label_,
	    *search_field_,
	    *case_sensitive_check_,
	    *regex_check_,
	    *search_col_key_,
	    *search_col_original_,
	    *search_col_translation_);

	if (!find_replace_)
		find_replace_ = new find_replace_t(*table_model_, active_doc_);
}

void main_window_t::connect_menu_signals()
{
	connect(
	    encoding_group_,
	    &QActionGroup::triggered,
	    this,
	    [this](QAction * act) { on_encoding_changed(act->data().toInt()); });

	connect(
	    spelling_group_,
	    &QActionGroup::triggered,
	    this,
	    [this](QAction * act) { on_spell_lang_changed(act->data().toInt()); });

	connect(save_action_, &QAction::triggered, this, &main_window_t::on_save);
	connect(save_all_action_, &QAction::triggered, this, &main_window_t::on_save_all);
	connect(quit_action_, &QAction::triggered, this, &QWidget::close);
	connect(find_action_, &QAction::triggered, this, &main_window_t::on_find);
	connect(escape_action_, &QAction::triggered, this, &main_window_t::on_escape);

	connect(sidebar_toggle_, &QAction::toggled, left_splitter_, &QWidget::setVisible);
	connect(bottom_panel_toggle_, &QAction::toggled, editor_view_, &QWidget::setVisible);
	connect(whitespace_check_, &QAction::toggled, this, &main_window_t::on_whitespace_toggled);

	connect(
	    grammar_check_,
	    &QAction::toggled,
	    this,
	    [this]()
	{
		if (editor_controller_.current_row() < 0)
			return;

		const auto * row_data = table_model_->row_at(editor_controller_.current_row());
		auto type = row_data ? row_data->type : tools_t::rec_type_t::info;
		extra_sel_translation_.grammar = grammar_check_->isChecked()
		                                     ? grammar_checker_.check(editor_view_->translation_editor(), type)
		                                     : QList<QTextEdit::ExtraSelection> {};
		apply_extra_selections(editor_view_->translation_editor(), extra_sel_translation_);
	});

	connect(
	    add_folder_action_,
	    &QAction::triggered,
	    this,
	    [this]()
	{
		const auto folder = QFileDialog::getExistingDirectory(this, "Add Folder");
		if (folder.isEmpty())
			return;

		const auto path = folder.toStdString();
		const auto & roots = config_.workspace_roots;
		if (std::find(roots.begin(), roots.end(), path) != roots.end())
			return;

		config_.workspace_roots.push_back(path);
		scan_workspace();
		save_config();
		update_watcher_paths();
	});

	connect(
	    import_archive_action_,
	    &QAction::triggered,
	    this,
	    [this]()
	{
		const auto archive_path = QFileDialog::getOpenFileName(this, "Import Archive", "", "Archives (*.zip *.rar)");

		if (archive_path.isEmpty())
			return;

		const auto app_dir = QCoreApplication::applicationDirPath();
		const QString sevenzip = app_dir + "/7za.exe";
		if (!QFile::exists(sevenzip))
		{
			QMessageBox::critical(this, "Error", "7za.exe not found next to the application");
			return;
		}

		const auto archive_name = QFileInfo(archive_path).completeBaseName();
		const auto target_dir = app_dir + "/workspace/" + archive_name;
		QDir().mkpath(target_dir);

		QProcess proc;
		proc.start(sevenzip, { "x", archive_path, "-o" + target_dir, "-y" });
		proc.waitForFinished(60000);

		if (proc.exitCode() != 0)
		{
			QMessageBox::critical(
			    this, "Extraction Error", "Failed to extract archive:\n" + proc.readAllStandardError());
			return;
		}

		scan_workspace();
		update_watcher_paths();
	});

	connect(
	    rescan_timer_,
	    &QTimer::timeout,
	    this,
	    [this]()
	{
		scan_workspace();

		if (active_doc_ && !QFile::exists(QString::fromStdString(active_doc_->path())))
		{
			const auto path = active_doc_->path();
			switch_document(nullptr);
			session_.close(path);
			filter_states_.erase(path);
			rebuild_annotations();
			last_annotation_version_ = session_.dict_version();
		}
	});
	connect(fs_watcher_, &QFileSystemWatcher::directoryChanged, rescan_timer_, qOverload<>(&QTimer::start));

	connect(
	    find_replace_dialog_,
	    &find_replace_dialog_t::find_next_requested,
	    this,
	    [this](const QString & query, bool case_sensitive, bool regex_mode)
	{
		auto result =
		    find_replace_->find_next(query.toStdString(), case_sensitive, regex_mode, editor_controller_.current_row());

		if (result.found)
			on_row_selected(result.row);
		else
			statusBar()->showMessage("No match found", 3000);
	});

	connect(
	    find_replace_dialog_,
	    &find_replace_dialog_t::replace_requested,
	    this,
	    [this](const QString & query, const QString & replacement, bool case_sensitive, bool regex_mode)
	{
		auto result = find_replace_->replace_current(
		    query.toStdString(),
		    replacement.toStdString(),
		    case_sensitive,
		    regex_mode,
		    editor_controller_.current_row());

		if (!result.replaced)
			return;

		set_unsaved_changes(true);
		table_model_->update_row(editor_controller_.current_row(), result.new_text, result.status);
		load_record(editor_controller_.current_row());

		emit find_replace_dialog_->find_next_requested(query, case_sensitive, regex_mode);
	});

	connect(
	    find_replace_dialog_,
	    &find_replace_dialog_t::replace_all_requested,
	    this,
	    [this](const QString & query, const QString & replacement, bool case_sensitive, bool regex_mode)
	{
		auto result =
		    find_replace_->replace_all(query.toStdString(), replacement.toStdString(), case_sensitive, regex_mode);

		if (result.count > 0)
		{
			set_unsaved_changes(true);
			rebuild_table();
			if (editor_controller_.current_row() >= 0)
				load_record(editor_controller_.current_row());
		}

		statusBar()->showMessage(QString("Replaced in %1 entries").arg(result.count), 5000);
	});
}

void main_window_t::connect_sidebar_signals()
{
	connect(sidebar_, &sidebar_view_t::item_clicked, this, &main_window_t::on_item_clicked);
	connect(sidebar_, &sidebar_view_t::operation_requested, this, &main_window_t::on_operation_requested);
	connect(sidebar_, &sidebar_view_t::save_requested, this, &main_window_t::on_save_requested);
	connect(sidebar_, &sidebar_view_t::unload_requested, this, &main_window_t::on_unload_requested);
	connect(sidebar_, &sidebar_view_t::delete_requested, this, &main_window_t::on_delete_requested);

	connect(
	    annotations_view_,
	    &annotations_view_t::rebuild_requested,
	    this,
	    [this]()
	{
		rebuild_annotations();
		last_annotation_version_ = session_.dict_version();
		if (editor_controller_.current_row() >= 0)
			load_record(editor_controller_.current_row());
	});

	connect(
	    sidebar_,
	    &sidebar_view_t::save_as_requested,
	    this,
	    [this](const std::string & path)
	{
		commit_current_edit();

		auto * yaml_doc = dynamic_cast<yaml_document_t *>(active_doc_);
		if (!yaml_doc)
			return;

		auto sep = path.find_last_of("/\\");
		auto default_dir = sep != std::string::npos ? path.substr(0, sep) : std::string {};

		auto save_path = QFileDialog::getSaveFileName(
		    this, "Save Translated YAML", QString::fromStdString(default_dir), "YAML files (*.yaml)");

		if (save_path.isEmpty())
			return;

		yaml_doc->export_to(save_path.toStdString());
		log_view_->append_log("save as", "saved \"" + save_path.toStdString() + "\"\r\n");

		update_sidebar_item(yaml_doc->path());

		switch_document(nullptr);
		set_unsaved_changes(false);
	});

	connect(
	    sidebar_,
	    &sidebar_view_t::remove_folder_requested,
	    this,
	    [this](const std::string & root_path)
	{
		auto & roots = config_.workspace_roots;
		roots.erase(std::remove(roots.begin(), roots.end(), root_path), roots.end());

		if (active_doc_)
		{
			const auto * fe = file_list_.get(active_doc_->path());
			if (fe && fe->root_path == root_path)
				switch_document(nullptr);
		}

		session_.close_if([this, &root_path](const document_t & doc)
		{
			const auto * fe = file_list_.get(doc.path());
			if (fe && fe->root_path == root_path)
			{
				filter_states_.erase(doc.path());
				return true;
			}
			return false;
		});

		rebuild_annotations();
		last_annotation_version_ = session_.dict_version();
		scan_workspace();
		save_config();
		update_watcher_paths();
	});

	connect(
	    sidebar_,
	    &sidebar_view_t::delete_folder_requested,
	    this,
	    [this](const std::string & folder_path)
	{
		auto sep = folder_path.find_last_of("/\\");
		auto folder_name = sep != std::string::npos ? folder_path.substr(sep + 1) : folder_path;

		auto answer = QMessageBox::question(
		    this,
		    "Delete Folder",
		    QString("Delete \"%1\" and all its contents from disk?").arg(QString::fromStdString(folder_name)),
		    QMessageBox::Yes | QMessageBox::No);

		if (answer != QMessageBox::Yes)
			return;

		if (active_doc_)
		{
			auto folder_norm = string_utils::normalize_path(folder_path);
			const auto & doc_path = active_doc_->path();
			if (doc_path.find(folder_norm + "/") == 0 || doc_path == folder_norm)
				switch_document(nullptr);
		}

		auto folder_norm = string_utils::normalize_path(folder_path);

		session_.close_if([this, &folder_norm](const document_t & doc)
		{
			const auto & p = doc.path();
			if (p.find(folder_norm + "/") == 0 || p == folder_norm)
			{
				filter_states_.erase(p);
				return true;
			}
			return false;
		});

		rebuild_annotations();
		last_annotation_version_ = session_.dict_version();
		QDir(QString::fromStdString(folder_path)).removeRecursively();
		scan_workspace();
	});

	connect(
	    history_view_,
	    &history_view_t::revert_requested,
	    this,
	    [this](size_t history_index)
	{
		if (editor_controller_.current_row() < 0)
			return;

		const auto * row_data = table_model_->row_at(editor_controller_.current_row());
		if (!row_data)
			return;

		auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
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

		const auto history = edit_history_.get_history(row_data->type, row_data->key_text);
		if (history_index >= history.size())
			return;

		std::string current_value = record->new_text;
		record->new_text = history[history_index].value;
		dict_doc->set_dirty(true);
		dict_doc->modified_records_insert(row_data->type, index_it->second);
		set_unsaved_changes(true);

		edit_history_.record_change(row_data->type, row_data->key_text, current_value, record->new_text);

		table_model_->update_row(editor_controller_.current_row(), record->new_text, record->status);
		load_record(editor_controller_.current_row());
	});
}

void main_window_t::connect_editor_signals()
{
	connect(table_view_, &record_table_view_t::row_selected, this, &main_window_t::on_row_selected);

	connect(
	    table_view_,
	    &record_table_view_t::batch_status_change_requested,
	    this,
	    [this](const QList<int> & rows, const QString & new_status)
	{
		auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
		if (!dict_doc)
			return;

		auto & data = dict_doc->data_mut();

		for (int row : rows)
		{
			auto * row_data = table_model_->row_at(row);
			if (!row_data)
				continue;

			auto it = data.find(row_data->type);
			if (it == data.end())
				continue;

			if (row_data->record_index >= it->second.records.size())
				continue;

			it->second.records[row_data->record_index].status = string_to_status(new_status.toStdString());
			table_model_->update_row(row, row_data->new_text, string_to_status(new_status.toStdString()));
		}

		dict_doc->set_dirty(true);
		set_unsaved_changes(true);
		update_status_counts();
	});

	connect(editor_view_, &editor_view_t::text_changed, this, &main_window_t::on_translation_changed);

	connect(
	    editor_view_,
	    &editor_view_t::apply_clicked,
	    this,
	    [this]()
	{
		if (editor_controller_.current_row() < 0)
			return;

		commit_current_edit();

		int row_count = table_model_->rowCount();
		int next_row = -1;
		for (int i = editor_controller_.current_row() + 1; i < row_count; ++i)
		{
			const auto * r = table_model_->row_at(i);
			if (r && r->status != status_t::propagated)
			{
				next_row = i;
				break;
			}
		}

		if (next_row < 0)
		{
			next_row = editor_controller_.current_row() + 1;
			if (next_row >= row_count)
				next_row = row_count - 1;
		}

		if (next_row >= 0 && next_row != editor_controller_.current_row())
		{
			auto idx = table_model_->index(next_row, 0);
			table_view_->selectionModel()->setCurrentIndex(
			    idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
			on_row_selected(next_row);
			auto cursor = editor_view_->translation_editor()->textCursor();
			cursor.movePosition(QTextCursor::End);
			editor_view_->translation_editor()->setTextCursor(cursor);
			editor_view_->translation_editor()->setFocus();
		}
	});

	connect(
	    editor_view_->translation_editor(),
	    &translation_edit_view_t::navigate_next,
	    this,
	    [this]()
	{
		if (editor_controller_.current_row() < 0)
			return;

		commit_current_edit();

		int row_count = table_model_->rowCount();
		int next_row = -1;
		for (int i = editor_controller_.current_row() + 1; i < row_count; ++i)
		{
			const auto * r = table_model_->row_at(i);
			if (r && r->status != status_t::propagated)
			{
				next_row = i;
				break;
			}
		}

		if (next_row < 0)
		{
			next_row = editor_controller_.current_row() + 1;
			if (next_row >= row_count)
				next_row = row_count - 1;
		}

		if (next_row >= 0 && next_row != editor_controller_.current_row())
		{
			auto idx = table_model_->index(next_row, 0);
			table_view_->selectionModel()->setCurrentIndex(
			    idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
			on_row_selected(next_row);
			auto cursor = editor_view_->translation_editor()->textCursor();
			cursor.movePosition(QTextCursor::End);
			editor_view_->translation_editor()->setTextCursor(cursor);
			editor_view_->translation_editor()->setFocus();
		}
	});

	connect(
	    editor_view_->translation_editor(),
	    &translation_edit_view_t::navigate_prev,
	    this,
	    [this]()
	{
		if (editor_controller_.current_row() <= 0)
			return;

		commit_current_edit();

		int prev_row = editor_controller_.current_row() - 1;
		auto idx = table_model_->index(prev_row, 0);
		table_view_->selectionModel()->setCurrentIndex(
		    idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
		on_row_selected(prev_row);
		auto cursor = editor_view_->translation_editor()->textCursor();
		cursor.movePosition(QTextCursor::End);
		editor_view_->translation_editor()->setTextCursor(cursor);
		editor_view_->translation_editor()->setFocus();
	});

	connect(
	    translation_tab_,
	    &translation_suggestion_view_t::translate_all_requested,
	    this,
	    [this]()
	{
		auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
		if (!dict_doc)
		{
			translation_tab_->append_log("[error] no dictionary loaded\n");
			return;
		}

		auto * provider = translation_tab_->ct2_provider();
		if (!provider->is_available())
		{
			translation_tab_->append_log("[error] model not loaded\n");
			return;
		}

		start_batch_translation(dict_doc);
	});
}

void main_window_t::connect_search_signals()
{
	connect(search_field_, &QLineEdit::textChanged, this, &main_window_t::on_search_changed);
	connect(case_sensitive_check_, &QPushButton::toggled, this, [this]() { on_case_sensitive_changed(0); });
	connect(regex_check_, &QPushButton::toggled, this, [this]() { on_search_changed(search_query_); });

	auto on_search_col_changed = [this]() { on_search_changed(search_query_); };
	connect(search_col_key_, &QPushButton::toggled, this, on_search_col_changed);
	connect(search_col_original_, &QPushButton::toggled, this, on_search_col_changed);
	connect(search_col_translation_, &QPushButton::toggled, this, on_search_col_changed);

	connect(filter_tree_view_, &filter_tree_view_t::filters_changed, this, &main_window_t::on_filters_changed);
	connect(
	    filter_tree_view_,
	    &filter_tree_view_t::all_reset_requested,
	    this,
	    [this]()
	{
		status_filter_.clear();
		status_filter_view_->set_filter_state(status_filter_);
	});
	connect(
	    status_filter_view_, &status_filter_view_t::filters_changed, this, &main_window_t::on_status_filters_changed);
}
