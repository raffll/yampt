#include "main_window.hpp"
#include "annotation_highlighter.hpp"
#include "table_builder.hpp"
#include "annotations_panel.hpp"
#include "book_preview.hpp"
#include "composite_highlighter.hpp"
#include "ctranslate2_provider.hpp"
#include "dict_document.hpp"
#include "display_name.hpp"
#include "plugin_document.hpp"
#include "dict_selection_dialog.hpp"
#include "editor_panel.hpp"
#include "filter_tree.hpp"
#include "find_replace_dialog.hpp"
#include "first_run_dialog.hpp"
#include "grammar_checker.hpp"
#include "history_panel.hpp"
#include "hyperlink_highlighter.hpp"
#include "log_tab.hpp"
#include "record_table_view.hpp"
#include "sidebar_widget.hpp"
#include "spell_context_menu.hpp"
#include "status_filter_bar.hpp"
#include "translation_suggestion_tab.hpp"
#include "validation_indicator.hpp"
#include "yaml_document.hpp"

#include "../yampt/dict_merger.hpp"
#include "../yampt/dict_writer.hpp"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDialogButtonBox>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextDocument>
#include <QPlainTextEdit>
#include <QTextOption>
#include <QToolBar>
#include <QTimer>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <unordered_map>

main_window_t::main_window_t(QWidget * parent)
    : QMainWindow(parent)
    , session_(current_codepage_)
    , editor_controller_(history_manager_, validation_manager_, annotation_manager_)
{
	setWindowTitle("yampt.translator");
	resize(1280, 720);
	setMinimumSize(800, 600);

	type_filter_ = {
		tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
		tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
		tools_t::rec_type_t::indx, tools_t::rec_type_t::sctx,
	};

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

	auto * quit_action = new QAction("&Quit", this);
	quit_action->setShortcut(QKeySequence("Alt+F4"));
	file_menu->addAction(quit_action);

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
	connect(encoding_group_, &QActionGroup::triggered, this, [this](QAction * act) {
		on_encoding_changed(act->data().toInt());
	});

	spelling_menu_ = view_menu->addMenu("&Spelling");
	spelling_group_ = new QActionGroup(this);
	auto * none_act = spelling_menu_->addAction("None");
	none_act->setCheckable(true);
	none_act->setChecked(true);
	none_act->setData(0);
	spelling_group_->addAction(none_act);

	auto * toolbar = new QToolBar(this);
	toolbar->setMovable(false);

	search_label_ = new QLabel("Filter by:", this);
	search_label_->setStyleSheet("QLabel:disabled { color: rgb(180,180,180); }");
	toolbar->addWidget(search_label_);
	search_field_ = new QLineEdit(this);
	search_field_->setPlaceholderText("Search...");
	toolbar->addWidget(search_field_);

	static const QString toggle_style =
	    "QPushButton { border: 1px solid #bbb; border-radius: 2px; padding: 2px 6px; background: #f0f0f0; }"
	    "QPushButton:checked { background: #cde; border-color: #89a; }"
	    "QPushButton:disabled { color: rgb(180,180,180); }";

	case_sensitive_check_ = new QPushButton("Aa", this);
	case_sensitive_check_->setCheckable(true);
	case_sensitive_check_->setFlat(false);
	case_sensitive_check_->setStyleSheet(toggle_style);
	toolbar->addWidget(case_sensitive_check_);

	regex_check_ = new QPushButton(".*", this);
	regex_check_->setCheckable(true);
	regex_check_->setStyleSheet(toggle_style);
	toolbar->addWidget(regex_check_);

	search_col_key_ = new QPushButton("Key", this);
	search_col_key_->setCheckable(true);
	search_col_key_->setChecked(true);
	search_col_key_->setStyleSheet(toggle_style);
	toolbar->addWidget(search_col_key_);

	search_col_original_ = new QPushButton("Original", this);
	search_col_original_->setCheckable(true);
	search_col_original_->setChecked(true);
	search_col_original_->setStyleSheet(toggle_style);
	toolbar->addWidget(search_col_original_);

	search_col_translation_ = new QPushButton("Translation", this);
	search_col_translation_->setCheckable(true);
	search_col_translation_->setChecked(true);
	search_col_translation_->setStyleSheet(toggle_style);
	toolbar->addWidget(search_col_translation_);

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

	auto * central_widget = new QWidget(this);
	auto * central_layout = new QVBoxLayout(central_widget);
	central_layout->setContentsMargins(0, 0, 0, 0);
	central_layout->setSpacing(4);

	central_layout->addWidget(toolbar);

	filter_tree_ = new filter_tree_t(this);
	status_filter_bar_ = new status_filter_bar_t(central_widget);
	central_layout->addWidget(status_filter_bar_);

	central_splitter_ = new QSplitter(Qt::Horizontal, central_widget);
	central_layout->addWidget(central_splitter_, 1);

	setCentralWidget(central_widget);

	left_splitter_ = new QSplitter(Qt::Vertical, central_splitter_);

	left_tabs_ = new QTabWidget(left_splitter_);
	sidebar_ = new sidebar_widget_t(left_tabs_);
	left_tabs_->addTab(sidebar_, "Files");
	left_tabs_->addTab(filter_tree_, "Filters");
	left_splitter_->addWidget(left_tabs_);

	info_tabs_ = new QTabWidget(left_splitter_);
	annotations_panel_ = new annotations_panel_t(info_tabs_);
	history_panel_ = new history_panel_t(info_tabs_);
	translation_tab_ = new translation_suggestion_tab_t(info_tabs_);
	translation_tab_->set_models_dir((QCoreApplication::applicationDirPath() + "/models").toStdString());
	find_replace_dialog_ = new find_replace_dialog_t(this);
	find_replace_dialog_->setVisible(false);
	info_tabs_->addTab(annotations_panel_, "Annotations");
	info_tabs_->addTab(history_panel_, "History");
	info_tabs_->addTab(translation_tab_, "Translate");
	left_splitter_->addWidget(info_tabs_);

	right_splitter_ = new QSplitter(Qt::Vertical, central_splitter_);

	auto * right_top_widget = new QWidget(right_splitter_);
	auto * right_top_layout = new QVBoxLayout(right_top_widget);
	right_top_layout->setContentsMargins(0, 0, 0, 0);
	right_top_layout->setSpacing(0);

	record_tabs_ = new QTabWidget(right_top_widget);
	table_model_ = new record_table_model_t(this);
	table_view_ = new record_table_view_t(record_tabs_);
	table_view_->setModel(table_model_);
	book_preview_ = new book_preview_t(record_tabs_);
	log_tab_ = new log_tab_t(record_tabs_);
	record_tabs_->addTab(table_view_, "Records");
	record_tabs_->addTab(book_preview_, "Book Preview");
	record_tabs_->addTab(log_tab_, "Log");
	right_top_layout->addWidget(record_tabs_, 1);

	right_splitter_->addWidget(right_top_widget);

	editor_panel_ = new editor_panel_t(right_splitter_);
	right_splitter_->addWidget(editor_panel_);

	active_file_label_ = new QLabel(this);
	statusBar()->addWidget(active_file_label_, 1);

	progress_label_ = new QLabel(this);
	statusBar()->addPermanentWidget(progress_label_);

	validation_indicator_ = new validation_indicator_t(this);
	statusBar()->addPermanentWidget(validation_indicator_);

	central_splitter_->addWidget(left_splitter_);
	central_splitter_->addWidget(right_splitter_);
	central_splitter_->setSizes({ 250, 1030 });

	hl_original_ = new composite_highlighter_t(editor_panel_->original_view()->document());
	hl_adapted_ = new composite_highlighter_t(editor_panel_->adapted_from_view()->document());
	hl_translation_ = new composite_highlighter_t(editor_panel_->translation_editor()->document());
	hl_translation_->set_translation_mode(true);

	spell_menu_ = new spell_context_menu_t(&spell_checker_, hl_translation_);

	connect(sidebar_toggle_, &QAction::toggled, left_splitter_, &QWidget::setVisible);
	connect(bottom_panel_toggle_, &QAction::toggled, editor_panel_, &QWidget::setVisible);
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
		                                     ? grammar_checker_.check(editor_panel_->translation_editor(), type)
		                                     : QList<QTextEdit::ExtraSelection> {};
		apply_extra_selections(editor_panel_->translation_editor(), extra_sel_translation_);
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

		const QString sevenzip = "C:/Program Files/7-Zip/7z.exe";
		if (!QFile::exists(sevenzip))
		{
			QMessageBox::critical(this, "Error", "7-Zip not found at C:\\Program Files\\7-Zip\\7z.exe");
			return;
		}

		const auto archive_name = QFileInfo(archive_path).completeBaseName();
		const auto app_dir = QCoreApplication::applicationDirPath();
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

		const auto workspace = (app_dir + "/workspace").toStdString();
		scan_workspace();
		update_watcher_paths();
	});

	connect(save_action_, &QAction::triggered, this, &main_window_t::on_save);
	connect(save_all_action_, &QAction::triggered, this, &main_window_t::on_save_all);
	connect(quit_action, &QAction::triggered, this, &QWidget::close);
	connect(find_action_, &QAction::triggered, this, &main_window_t::on_find);
	connect(escape_action_, &QAction::triggered, this, &main_window_t::on_escape);

	{
		if (!find_replace_service_)
			find_replace_service_ = new find_replace_service_t(*table_model_, active_doc_);

		connect(
		    find_replace_dialog_,
		    &find_replace_dialog_t::find_next_requested,
		    this,
		    [this](const QString & query, bool case_sensitive, bool regex_mode)
		{
			auto result = find_replace_service_->find_next(
			    query.toStdString(), case_sensitive, regex_mode, editor_controller_.current_row());

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
			auto result = find_replace_service_->replace_current(
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
			auto result = find_replace_service_->replace_all(
			    query.toStdString(), replacement.toStdString(), case_sensitive, regex_mode);

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

	connect(search_field_, &QLineEdit::textChanged, this, &main_window_t::on_search_changed);
	connect(case_sensitive_check_, &QPushButton::toggled, this, [this]() { on_case_sensitive_changed(0); });
	connect(regex_check_, &QPushButton::toggled, this, [this]() { on_search_changed(search_query_); });

	auto on_search_col_changed = [this]() { on_search_changed(search_query_); };
	connect(search_col_key_, &QPushButton::toggled, this, on_search_col_changed);
	connect(search_col_original_, &QPushButton::toggled, this, on_search_col_changed);
	connect(search_col_translation_, &QPushButton::toggled, this, on_search_col_changed);

	connect(translation_tab_, &translation_suggestion_tab_t::translate_all_requested, this, [this]()
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

		auto & data = dict_doc->data_mut();
		for (auto & [type, chapter] : data)
		{
			for (size_t i = 0; i < chapter.records.size(); ++i)
			{
				if (chapter.records[i].status == "untranslated" && !chapter.records[i].old_text.empty())
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

		translation_tab_->append_log(
		    "[info] translating " + std::to_string(state->work_items.size()) + " entries\n");

		auto * timer = new QTimer(this);
		timer->setInterval(0);
		connect(timer, &QTimer::timeout, this, [this, state, dict_doc, provider, timer]()
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
				    "[info] done: translated=" + std::to_string(state->translated_count) +
				    " glossary=" + std::to_string(state->glossary_count) +
				    " errors=" + std::to_string(state->error_count) + "\n");
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

			auto glossary_applied = annotation_manager_.apply_glossary(result.text);
			bool had_glossary = (glossary_applied != result.text);

			record.new_text = glossary_applied;
			record.status = "model";
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
	});

	connect(
	    spelling_group_,
	    &QActionGroup::triggered,
	    this,
	    [this](QAction * act) { on_spell_lang_changed(act->data().toInt()); });

	connect(sidebar_, &sidebar_widget_t::item_clicked, this, &main_window_t::on_item_clicked);
	connect(sidebar_, &sidebar_widget_t::operation_requested, this, &main_window_t::on_operation_requested);
	connect(sidebar_, &sidebar_widget_t::save_requested, this, &main_window_t::on_save_requested);
	connect(
	    sidebar_,
	    &sidebar_widget_t::save_as_requested,
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
		log_tab_->append_log("save as", "saved \"" + save_path.toStdString() + "\"\r\n");

		update_sidebar_item(yaml_doc->path());

		switch_document(nullptr);
		set_unsaved_changes(false);
	});
	connect(sidebar_, &sidebar_widget_t::unload_requested, this, &main_window_t::on_unload_requested);
	connect(sidebar_, &sidebar_widget_t::delete_requested, this, &main_window_t::on_delete_requested);
	connect(
	    sidebar_,
	    &sidebar_widget_t::remove_folder_requested,
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
	    &sidebar_widget_t::delete_folder_requested,
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
			auto folder_norm = folder_path;
			std::replace(folder_norm.begin(), folder_norm.end(), '\\', '/');
			const auto & doc_path = active_doc_->path();
			if (doc_path.find(folder_norm + "/") == 0 || doc_path == folder_norm)
				switch_document(nullptr);
		}

		auto folder_norm = folder_path;
		std::replace(folder_norm.begin(), folder_norm.end(), '\\', '/');

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

			it->second.records[row_data->record_index].status = new_status.toStdString();
			table_model_->update_row(row, row_data->new_text, new_status.toStdString());
		}

		dict_doc->set_dirty(true);
		set_unsaved_changes(true);
		update_status_counts();
	});

	connect(editor_panel_, &editor_panel_t::text_changed, this, &main_window_t::on_translation_changed);
	connect(
	    editor_panel_,
	    &editor_panel_t::apply_clicked,
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
			if (r && r->status != "propagated")
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
			auto cursor = editor_panel_->translation_editor()->textCursor();
			cursor.movePosition(QTextCursor::End);
			editor_panel_->translation_editor()->setTextCursor(cursor);
			editor_panel_->translation_editor()->setFocus();
		}
	});

	connect(
	    editor_panel_->translation_editor(),
	    &editor_text_edit_t::navigate_next,
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
			if (r && r->status != "propagated")
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
			auto cursor = editor_panel_->translation_editor()->textCursor();
			cursor.movePosition(QTextCursor::End);
			editor_panel_->translation_editor()->setTextCursor(cursor);
			editor_panel_->translation_editor()->setFocus();
		}
	});

	connect(
	    editor_panel_->translation_editor(),
	    &editor_text_edit_t::navigate_prev,
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
		auto cursor = editor_panel_->translation_editor()->textCursor();
		cursor.movePosition(QTextCursor::End);
		editor_panel_->translation_editor()->setTextCursor(cursor);
		editor_panel_->translation_editor()->setFocus();
	});

	connect(filter_tree_, &filter_tree_t::filters_changed, this, &main_window_t::on_filters_changed);
	connect(
	    filter_tree_,
	    &filter_tree_t::all_reset_requested,
	    this,
	    [this]()
	{
		status_filter_.clear();
		status_filter_bar_->set_filter_state(status_filter_);
	});
	connect(status_filter_bar_, &status_filter_bar_t::filters_changed, this, &main_window_t::on_status_filters_changed);

	connect(
	    history_panel_,
	    &history_panel_t::revert_requested,
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

		const auto history = history_manager_.get_history(row_data->type, row_data->key_text);
		if (history_index >= history.size())
			return;

		std::string current_value = record->new_text;
		record->new_text = history[history_index].value;
		dict_doc->set_dirty(true);
		dict_doc->modified_records_insert(row_data->type, index_it->second);
		set_unsaved_changes(true);

		history_manager_.record_change(row_data->type, row_data->key_text, current_value, record->new_text);

		table_model_->update_row(editor_controller_.current_row(), record->new_text, record->status);
		load_record(editor_controller_.current_row());
	});

	fs_watcher_ = new QFileSystemWatcher(this);
	rescan_timer_ = new QTimer(this);
	rescan_timer_->setSingleShot(true);
	rescan_timer_->setInterval(200);
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

	scan_spell_dictionaries();

	table_display_ = std::make_unique<table_display_t>(
	    *filter_tree_,
	    *status_filter_bar_,
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

	const auto config_path = QCoreApplication::applicationDirPath() + "/yampt.translator.ini";
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
	setWindowTitle(has_unsaved_changes_ ? "yampt.translator *" : "yampt.translator");
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

	log_tab_->append_log("save", "saved \"" + active_doc_->path() + "\"\r\n");

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
		log_tab_->append_log("save all", log_msg);

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

	search_engine_t::config_t cfg;
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
	search_engine_.set_config(cfg);

	rebuild_table();
}

void main_window_t::on_case_sensitive_changed(int /*state*/)
{
	on_search_changed(search_query_);
}

void main_window_t::on_filters_changed()
{
	type_filter_ = filter_tree_->get_active_types();
	type_filter_solo_ = filter_tree_->has_sub_type_filter();
	rebuild_table();
}

void main_window_t::on_status_filters_changed()
{
	status_filter_ = status_filter_bar_->get_active_statuses();
	rebuild_table();
}

void main_window_t::clear_editor_panels()
{
	editor_panel_->original_view()->clear();
	editor_panel_->translation_editor()->clear();
	editor_panel_->clear_adapted_from();
	validation_indicator_->clear();
	annotations_panel_->clear();
	history_panel_->clear();
	book_preview_->clear();
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
		filter_tree_->set_display_mode(filter_tree_t::display_mode_t::full);
		if (session_.dict_version() != last_annotation_version_)
		{
			rebuild_annotations();
			last_annotation_version_ = session_.dict_version();
		}
	}
	else if (dynamic_cast<plugin_document_t *>(active_doc_))
	{
		filter_tree_->set_display_mode(filter_tree_t::display_mode_t::empty);
		filter_tree_->setEnabled(false);
	}
	else
	{
		filter_tree_->set_display_mode(filter_tree_t::display_mode_t::all_only);
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
		return;
	}

	if (dynamic_cast<plugin_document_t *>(active_doc_))
	{
		table_display_->clear();
		editor_controller_.set_current_row(-1);
		return;
	}

	auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
	if (!dict_doc)
	{
		const auto raw_rows = active_doc_->build_rows();

		std::map<std::string, size_t> total_status_counts;
		std::map<std::string, size_t> filtered_status_counts;

		for (const auto & row : raw_rows)
		{
			total_status_counts[row.status]++;

			if (search_engine_.has_query() && !search_engine_.matches(row))
				continue;

			filtered_status_counts[row.status]++;
		}

		std::vector<table_row_t> rows;
		for (const auto & row : raw_rows)
		{
			if (!status_filter_.empty() && status_filter_.count(row.status) == 0)
				continue;

			if (search_engine_.has_query() && !search_engine_.matches(row))
				continue;

			rows.push_back(row);
		}

		int total = active_doc_->total_count();
		int translated = active_doc_->translated_count();
		table_display_->apply_yaml(
		    std::move(rows), total, translated, active_doc_->path(), filtered_status_counts, total_status_counts);
		editor_controller_.set_current_row(-1);
		return;
	}

	auto result = build_filtered_rows(
	    dict_doc->data(),
	    dict_doc->kind(),
	    type_filter_,
	    filter_tree_->get_active_sub_types(),
	    status_filter_,
	    search_engine_,
	    type_filter_solo_);

	table_display_->apply(std::move(result), dict_doc->path(), dict_doc->kind());
	editor_controller_.set_current_row(-1);
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

	const auto annotations = annotation_manager_.annotate(row_data->old_text, row_data->type);
	const auto current_text = editor_panel_->translation_editor()->toPlainText().toLower();

	struct highlight_t
	{
		int start;
		int length;
		bool is_hyperlink;
	};

	struct candidate_t
	{
		int start;
		int length;
		bool is_hyperlink;
	};

	std::vector<candidate_t> candidates;
	for (const auto & ann : annotations)
	{
		if (ann.new_text.empty())
			continue;

		bool is_hl = (ann.kind == annotation_t::dial_topic);
		const auto term = QString::fromStdString(ann.new_text).toLower();
		int pos = 0;
		while ((pos = current_text.indexOf(term, pos)) != -1)
		{
			candidates.push_back({ pos, static_cast<int>(term.length()), is_hl });
			pos += static_cast<int>(term.length());
		}
	}

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

	std::vector<bool> covered(current_text.length(), false);
	QList<QTextEdit::ExtraSelection> selections;

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

		QTextEdit::ExtraSelection sel;
		sel.format.setBackground(c.is_hyperlink ? QColor(200, 220, 255) : QColor(200, 240, 200));
		sel.cursor = editor_panel_->translation_editor()->textCursor();
		sel.cursor.setPosition(c.start);
		sel.cursor.setPosition(c.start + c.length, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

	extra_sel_translation_.annotations = selections;
	extra_sel_translation_.grammar = grammar_check_->isChecked()
	                                     ? grammar_checker_.check(editor_panel_->translation_editor(), row_data->type)
	                                     : QList<QTextEdit::ExtraSelection> {};
	apply_extra_selections(editor_panel_->translation_editor(), extra_sel_translation_);

	{
		auto doc_text = editor_panel_->translation_editor()->toPlainText();
		std::string log_msg = "grammar count=" + std::to_string(extra_sel_translation_.grammar.size()) + "\r\n";
		for (int i = 0; i < extra_sel_translation_.grammar.size(); ++i)
		{
			auto sel_cursor = extra_sel_translation_.grammar[i].cursor;
			int start = sel_cursor.anchor();
			int end = sel_cursor.position();
			auto slice = doc_text.mid(start, end - start);
			log_msg += "  [" + std::to_string(i) + "] pos=" + std::to_string(start) +
			           " len=" + std::to_string(end - start) + " text=\"" + slice.toStdString() + "\"\r\n";
		}
		log_tab_->append_log("grammar", log_msg);
	}
}

void main_window_t::commit_current_edit()
{
	if (editor_controller_.current_row() < 0)
		return;

	if (editor_controller_.is_loading())
		return;

	if (!editor_panel_)
		return;

	if (!active_doc_)
		return;

	if (active_doc_->is_read_only())
		return;

	const auto & current_text = editor_panel_->translation_editor()->toPlainText();
	if (current_text == editor_controller_.loaded_text())
		return;

	const auto * row_data = table_model_->row_at(editor_controller_.current_row());
	if (!row_data)
		return;

	std::string new_text_str;
	if (editor_panel_->has_script_template())
	{
		new_text_str = editor_panel_->reconstruct_script_line();

		const auto lines = current_text.split('\n');
		const size_t slot_count = editor_panel_->script_slot_count();

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
		const auto result = editor_controller_.commit(*dict_doc, *row_data, new_text_str);
		if (!result.success)
			return;

		if (result.propagated_count > 0)
		{
			statusBar()->showMessage(QString("Propagated to %1 entries").arg(result.propagated_count), 5000);

			table_model_->update_row(editor_controller_.current_row(), result.new_text, result.status);

			auto & data = dict_doc->data_mut();
			for (int i = 0; i < table_model_->rowCount(); ++i)
			{
				if (i == editor_controller_.current_row())
					continue;

				const auto * r = table_model_->row_at(i);
				if (!r)
					continue;

				auto chap_it = data.find(r->type);
				if (chap_it == data.end())
					continue;

				if (r->record_index >= chap_it->second.records.size())
					continue;

				const auto & rec = chap_it->second.records[r->record_index];
				if (rec.new_text != r->new_text || rec.status != r->status)
					table_model_->update_row(i, rec.new_text, rec.status);
			}

			set_unsaved_changes(active_doc_->is_dirty());
			editor_controller_.set_loaded_text(current_text);
			update_status_counts();
			return;
		}

		table_model_->update_row(editor_controller_.current_row(), result.new_text, result.status);
	}
	else
	{
		active_doc_->commit_edit(row_data->type, row_data->record_index, new_text_str);
		table_model_->update_row(editor_controller_.current_row(), new_text_str, "in_progress");

		auto * yaml_doc = dynamic_cast<yaml_document_t *>(active_doc_);
		if (yaml_doc)
			yaml_doc->save_tmp();
	}

	set_unsaved_changes(active_doc_->is_dirty());
	editor_controller_.set_loaded_text(current_text);
	update_status_counts();
}

void main_window_t::load_record(int row)
{
	editor_controller_.set_loading(true);

	if (!table_model_ || !editor_panel_)
	{
		editor_controller_.set_current_row(-1);
		editor_controller_.set_loading(false);
		return;
	}

	const auto * row_data = table_model_->row_at(row);
	if (!row_data)
	{
		editor_panel_->original_view()->clear();
		editor_panel_->translation_editor()->clear();
		validation_indicator_->clear();
		annotations_panel_->clear();
		history_panel_->clear();
		book_preview_->clear();
		editor_controller_.set_current_row(-1);
		editor_controller_.set_loading(false);
		return;
	}

	const auto load_result = active_doc_ ? editor_controller_.load(*active_doc_, *row_data) : editor_load_result_t {};

	if (row_data->type == tools_t::rec_type_t::sctx || row_data->type == tools_t::rec_type_t::bnam)
	{
		editor_panel_->load_script_entry(row_data->old_text, row_data->new_text);
		editor_panel_->translation_editor()->set_block_multiline(true);

		if (row_data->status == "untranslated")
		{
			int line_count = editor_panel_->script_slot_count();
			QString empty_lines;
			for (int i = 1; i < static_cast<int>(line_count); ++i)
				empty_lines += '\n';
			editor_panel_->translation_editor()->setPlainText(empty_lines);
		}
	}
	else
	{
		editor_panel_->original_view()->setPlainText(QString::fromStdString(row_data->old_text));
		editor_panel_->clear_script_template();

		if (row_data->status == "untranslated")
			editor_panel_->translation_editor()->setPlainText(QString());
		else
			editor_panel_->translation_editor()->setPlainText(QString::fromStdString(row_data->new_text));

		bool block =
		    (row_data->type == tools_t::rec_type_t::cell || row_data->type == tools_t::rec_type_t::dial ||
		     row_data->type == tools_t::rec_type_t::fnam);
		editor_panel_->translation_editor()->set_block_multiline(block);
	}

	editor_panel_->translation_editor()->setReadOnly(load_result.is_read_only);

	hl_original_->set_record_type(row_data->type);
	hl_adapted_->set_record_type(row_data->type);
	hl_translation_->set_record_type(row_data->type);

	const auto validation_result = validation_manager_.validate(row_data->type, row_data->new_text);
	validation_indicator_->update_validation(validation_result);

	if (row_data->type == tools_t::rec_type_t::text)
		book_preview_->set_html(row_data->old_text, row_data->new_text);
	else
		book_preview_->clear();

	const auto & annotations = load_result.annotations;

	annotations_panel_->update_annotations(
	    annotations, load_result.speaker_name, load_result.gender, load_result.enchantment);

	translation_tab_->set_source_text(row_data->old_text);

	if ((row_data->status == "adapted" || row_data->status == "changed") && !load_result.adapted_from.empty())
	{
		editor_panel_->set_adapted_from(load_result.adapted_from);
	}
	else
	{
		editor_panel_->clear_adapted_from();
	}

	struct highlight_t
	{
		int start;
		int length;
		bool is_hyperlink;
	};

	auto find_highlights = [](const QString & text_lower,
	                          const std::vector<annotation_t> & annotations,
	                          bool use_old) -> std::vector<highlight_t>
	{
		struct candidate_t
		{
			int start;
			int length;
			bool is_hyperlink;
		};

		std::vector<candidate_t> candidates;

		for (const auto & ann : annotations)
		{
			const auto & raw = use_old ? ann.old_text : ann.new_text;
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

		std::vector<bool> covered(text_lower.length(), false);
		std::vector<highlight_t> results;

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
	};

	const auto original_text_lower = editor_panel_->original_view()->toPlainText().toLower();
	const auto translation_text_lower = editor_panel_->translation_editor()->toPlainText().toLower();

	auto orig_highlights = find_highlights(original_text_lower, annotations, true);
	QList<QTextEdit::ExtraSelection> orig_selections;
	for (const auto & h : orig_highlights)
	{
		QTextEdit::ExtraSelection sel;
		sel.format.setBackground(h.is_hyperlink ? QColor(200, 220, 255) : QColor(200, 240, 200));
		sel.cursor = editor_panel_->original_view()->textCursor();
		sel.cursor.setPosition(h.start);
		sel.cursor.setPosition(h.start + h.length, QTextCursor::KeepAnchor);
		orig_selections.append(sel);
	}
	extra_sel_original_.annotations = orig_selections;
	extra_sel_original_.grammar.clear();
	extra_sel_original_.adapted_diff.clear();
	apply_extra_selections(editor_panel_->original_view(), extra_sel_original_);

	auto trans_highlights = find_highlights(translation_text_lower, annotations, false);
	QList<QTextEdit::ExtraSelection> trans_selections;

	{
		auto doc_text = editor_panel_->translation_editor()->toPlainText();
		std::string log_msg = "count=" + std::to_string(trans_highlights.size()) +
		                      " doc_len=" + std::to_string(doc_text.length()) + "\r\n";
		for (size_t i = 0; i < trans_highlights.size(); ++i)
		{
			const auto & h = trans_highlights[i];
			auto slice = doc_text.mid(h.start, h.length);
			log_msg += "  [" + std::to_string(i) + "] pos=" + std::to_string(h.start) +
			           " len=" + std::to_string(h.length) + " text=\"" + slice.toStdString() + "\"\r\n";
		}
		log_tab_->append_log("all highlights", log_msg);
	}

	for (const auto & h : trans_highlights)
	{
		QTextEdit::ExtraSelection sel;
		sel.format.setBackground(h.is_hyperlink ? QColor(200, 220, 255) : QColor(200, 240, 200));
		sel.cursor = QTextCursor(editor_panel_->translation_editor()->document());
		sel.cursor.setPosition(h.start);
		sel.cursor.setPosition(h.start + h.length, QTextCursor::KeepAnchor);
		trans_selections.append(sel);
	}
	extra_sel_translation_.annotations = trans_selections;
	extra_sel_translation_.grammar = grammar_check_->isChecked()
	                                     ? grammar_checker_.check(editor_panel_->translation_editor(), row_data->type)
	                                     : QList<QTextEdit::ExtraSelection> {};
	extra_sel_translation_.adapted_diff.clear();
	apply_extra_selections(editor_panel_->translation_editor(), extra_sel_translation_);

	{
		auto doc_text = editor_panel_->translation_editor()->toPlainText();
		std::string log_msg = "grammar count=" + std::to_string(extra_sel_translation_.grammar.size()) + "\r\n";
		for (int i = 0; i < static_cast<int>(extra_sel_translation_.grammar.size()); ++i)
		{
			auto sel_cursor = extra_sel_translation_.grammar[i].cursor;
			int start = sel_cursor.anchor();
			int end = sel_cursor.position();
			auto slice = doc_text.mid(start, end - start);
			log_msg += "  [" + std::to_string(i) + "] pos=" + std::to_string(start) +
			           " len=" + std::to_string(end - start) + " text=\"" + slice.toStdString() + "\"\r\n";
		}
		log_tab_->append_log("grammar", log_msg);

		auto text_str = doc_text.toStdString();
		auto spell_matches = spell_checker_.find_misspelled(text_str);
		std::string spell_msg = "spelling count=" + std::to_string(spell_matches.size()) + "\r\n";
		for (size_t i = 0; i < spell_matches.size(); ++i)
		{
			const auto & m = spell_matches[i];
			int qchar_start = QString::fromUtf8(text_str.data(), static_cast<int>(m.start)).length();
			int qchar_len = QString::fromUtf8(text_str.data() + m.start, static_cast<int>(m.end - m.start)).length();
			auto slice = doc_text.mid(qchar_start, qchar_len);
			spell_msg += "  [" + std::to_string(i) + "] byte_pos=" + std::to_string(m.start) +
			             " qchar_pos=" + std::to_string(qchar_start) + " len=" + std::to_string(qchar_len) +
			             " word=\"" + m.word +
			             "\""
			             " slice=\"" +
			             slice.toStdString() + "\"\r\n";
		}
		log_tab_->append_log("spelling", spell_msg);
	}

	extra_sel_adapted_.annotations.clear();
	extra_sel_adapted_.grammar.clear();
	extra_sel_adapted_.adapted_diff.clear();

	if (row_data->status == "adapted" && !load_result.adapted_from.empty())
	{
		extra_sel_adapted_.adapted_diff =
		    editor_panel_->highlight_adapted_diff(row_data->new_text, load_result.adapted_from, false);
	}
	else if (row_data->status == "changed" && !load_result.adapted_from.empty())
	{
		extra_sel_adapted_.adapted_diff =
		    editor_panel_->highlight_adapted_diff(row_data->old_text, load_result.adapted_from, true);
	}

	apply_extra_selections(editor_panel_->adapted_from_view(), extra_sel_adapted_);

	const auto history = history_manager_.get_history(row_data->type, row_data->key_text);
	history_panel_->update_history(history, !load_result.is_read_only);

	editor_controller_.set_loaded_text(editor_panel_->translation_editor()->toPlainText());
	editor_controller_.set_current_row(row);

	auto cursor = editor_panel_->translation_editor()->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor_panel_->translation_editor()->setTextCursor(cursor);

	editor_controller_.set_loading(false);
}

void main_window_t::on_whitespace_toggled(bool checked)
{
	if (!editor_panel_)
		return;

	QTextOption opt;
	if (checked)
		opt.setFlags(QTextOption::ShowTabsAndSpaces);

	editor_panel_->original_view()->document()->setDefaultTextOption(opt);
	editor_panel_->translation_editor()->document()->setDefaultTextOption(opt);
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
	validation_manager_.set_codepage(new_codepage);
	config_.encoding_index = index;
	save_config();

	statusBar()->showMessage("Encoding changed. Open documents keep their original encoding until re-opened.", 5000);
}

void main_window_t::rebuild_annotations()
{
	std::vector<dict_source_t> sources;
	for (auto * dict_doc : session_.all_dicts())
		sources.push_back({ &dict_doc->data(), dict_doc->path() });

	annotation_manager_.rebuild(sources);
}

void main_window_t::save_current_filter_state()
{
	if (!active_doc_)
		return;

	filter_state_t state;
	state.type_filter = type_filter_;
	state.sub_type_filter = filter_tree_->get_active_sub_types();
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
		filter_tree_->set_active_types(it->second.type_filter);
		filter_tree_->set_active_sub_types(it->second.sub_type_filter);
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
		filter_tree_->set_active_types(type_filter_);
		filter_tree_->set_active_sub_types({});
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

	const auto annotations = annotation_manager_.annotate(row_data->old_text, row_data->type);

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

	annotations_panel_->update_annotations(annotations, speaker_name, gender_str, enchantment_str);
}

void main_window_t::apply_extra_selections(editor_text_edit_t * editor, const extra_selections_state_t & state)
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

	auto result = build_filtered_rows(
	    dict_doc->data(),
	    dict_doc->kind(),
	    type_filter_,
	    filter_tree_->get_active_sub_types(),
	    status_filter_,
	    search_engine_,
	    type_filter_solo_);

	size_t total = 0;
	size_t total_translated = 0;
	for (const auto & [t, c] : result.counts.type_counts)
		total += c;
	for (const auto & [t, c] : result.counts.translated_counts)
		total_translated += c;

	filter_tree_->update_counts(result.counts.type_counts, result.counts.translated_counts);
	filter_tree_->update_sub_type_counts(result.counts.sub_type_total_counts, result.counts.sub_type_translated_counts);
	filter_tree_->set_total_count(total_translated, total);
	status_filter_bar_->update_counts(result.counts.filtered_status_counts, result.counts.total_status_counts);
}

void main_window_t::update_validation()
{
	if (editor_controller_.current_row() < 0)
		return;

	const auto * row_data = table_model_->row_at(editor_controller_.current_row());
	if (!row_data)
		return;

	const auto current_text = editor_panel_->translation_editor()->toPlainText().toStdString();
	const auto result = validation_manager_.validate(row_data->type, current_text);
	validation_indicator_->update_validation(result);
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
	const auto path = QCoreApplication::applicationDirPath() + "/yampt.translator.ini";
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
		editor_panel_->set_split_ratio(config_.split_ratio);

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

	config_.encoding_index = encoding_group_->checkedAction()
	    ? encoding_group_->actions().indexOf(encoding_group_->checkedAction())
	    : 2;
	config_.spell_lang_index = spelling_group_->checkedAction()
	    ? spelling_group_->actions().indexOf(spelling_group_->checkedAction())
	    : 0;

	config_.sidebar_visible = sidebar_toggle_->isChecked();
	config_.bottom_visible = bottom_panel_toggle_->isChecked();

	config_.split_ratio = static_cast<float>(editor_panel_->get_split_ratio());

	const auto col_widths = table_view_->get_column_widths();
	for (size_t i = 0; i < col_widths.size() && i < config_.column_widths.size(); ++i)
		config_.column_widths[i] = static_cast<float>(col_widths[i]);

	config_.active_dict_index = -1; // deprecated — use active_dict_path
	config_.active_dict_path = active_doc_ ? active_doc_->path() : std::string {};
	config_.translation_language_index = translation_tab_->language_index();
	config_.workspace_roots = file_list_.get_roots();

	const auto path = QCoreApplication::applicationDirPath() + "/yampt.translator.ini";
	config_.save(path.toStdString());
}

void main_window_t::on_plugin_operation(const std::string & plugin_path_arg, plugin_op_t op)
{
	const auto plugin_path = plugin_path_arg;
	const auto encoding = current_codepage_;

	auto path_sep = plugin_path.find_last_of("/\\");
	auto plugin_dir = path_sep != std::string::npos ? plugin_path.substr(0, path_sep) : std::string {};
	std::replace(plugin_dir.begin(), plugin_dir.end(), '\\', '/');

	executor_.set_output_dir(plugin_dir);

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
			return;

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
			return;

		auto * selected_item = tree->currentItem();
		if (!selected_item || !(selected_item->flags() & Qt::ItemIsSelectable))
			return;

		const auto native_path = selected_item->data(0, Qt::UserRole).toString().toStdString();
		const auto * native_entry = file_list_.get(native_path);

		std::string foreign_lang;
		const auto * foreign_entry = file_list_.get(plugin_path);
		if (foreign_entry)
			foreign_lang = foreign_entry->language_tag;

		std::string native_lang;
		if (native_entry)
			native_lang = native_entry->language_tag;

		result = executor_.make_base(plugin_path, native_path, foreign_lang, native_lang);
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

	auto sep = plugin_path.find_last_of("/\\");
	auto plugin_name = sep != std::string::npos ? plugin_path.substr(sep + 1) : plugin_path;

	std::string op_name;
	switch (op)
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

	log_tab_->append_log(op_name, result.log_text);
	record_tabs_->setCurrentWidget(log_tab_);
	scan_workspace();
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
	rebuild_sidebar();
}

void main_window_t::update_watcher_paths()
{
	const auto current = fs_watcher_->directories();
	if (!current.isEmpty())
		fs_watcher_->removePaths(current);

	QStringList paths;

	auto add_recursive = [&](const QString & dir)
	{
		QDir qdir(dir);
		if (!qdir.exists())
			return;

		paths.append(dir);

		QDirIterator it(dir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
		while (it.hasNext())
			paths.append(it.next());
	};

	const auto workspace = QCoreApplication::applicationDirPath() + "/workspace";
	add_recursive(workspace);

	for (const auto & root : config_.workspace_roots)
		add_recursive(QString::fromStdString(root));

	if (!paths.isEmpty())
		fs_watcher_->addPaths(paths);
}

std::vector<dict_selection_dialog_t::dict_entry_t> main_window_t::build_dict_entries(
    const std::string & source_dir) const
{
	std::set<std::string> seen;
	std::vector<dict_selection_dialog_t::dict_entry_t> entries;

	auto normalize = [](std::string p)
	{
		std::replace(p.begin(), p.end(), '\\', '/');
		std::transform(
		    p.begin(), p.end(), p.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return p;
	};

	auto extract_filename = [](const std::string & p)
	{
		auto sep = p.find_last_of("/\\");
		return sep != std::string::npos ? p.substr(sep + 1) : p;
	};

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
		    { extract_filename(dict_doc->path()),
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

	auto norm_path = path;
	std::replace(norm_path.begin(), norm_path.end(), '\\', '/');

	if (active_doc_ && active_doc_->path() == norm_path)
		return;

	auto * doc = session_.open(path);
	if (!doc)
		return;

	switch_document(doc);
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
	auto norm_path = path;
	std::replace(norm_path.begin(), norm_path.end(), '\\', '/');

	if (active_doc_ && active_doc_->path() == norm_path)
	{
		commit_current_edit();
		active_doc_->save();
		update_sidebar_item(active_doc_->path());

		log_tab_->append_log("save", "saved \"" + active_doc_->path() + "\"\r\n");

		if (!session_.has_any_unsaved())
			set_unsaved_changes(false);

		return;
	}

	auto * doc = session_.find(path);
	if (!doc)
		return;

	doc->save();
	update_sidebar_item(doc->path());

	log_tab_->append_log("save", "saved \"" + doc->path() + "\"\r\n");

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

	auto norm = path;
	std::replace(norm.begin(), norm.end(), '\\', '/');
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

	auto norm_del_path = path;
	std::replace(norm_del_path.begin(), norm_del_path.end(), '\\', '/');
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
