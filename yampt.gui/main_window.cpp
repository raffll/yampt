#include "main_window.hpp"
#include "annotation_highlighter.hpp"
#include "annotations_panel.hpp"
#include "book_preview.hpp"
#include "composite_highlighter.hpp"
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
#include "validation_indicator.hpp"
#include "yaml_l10n_writer.hpp"

#include "../yampt/dict_merger.hpp"
#include "../yampt/dict_writer.hpp"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
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
#include <QVBoxLayout>

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <unordered_map>

main_window_t::main_window_t(QWidget * parent)
    : QMainWindow(parent)
{
    setWindowTitle("yampt.gui");
    resize(1280, 720);
    setMinimumSize(800, 600);

    type_filter_ = {
        tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info,
        tools_t::rec_type_t::fnam, tools_t::rec_type_t::text, tools_t::rec_type_t::gmst,
        tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam, tools_t::rec_type_t::indx,
        tools_t::rec_type_t::bnam, tools_t::rec_type_t::sctx,
    };

    auto * file_menu = menuBar()->addMenu("&File");

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

    sidebar_toggle_ = new QAction("Toggle &Sidebar", this);
    sidebar_toggle_->setCheckable(true);
    sidebar_toggle_->setChecked(true);
    view_menu->addAction(sidebar_toggle_);

    bottom_panel_toggle_ = new QAction("Toggle &Bottom Panel", this);
    bottom_panel_toggle_->setCheckable(true);
    bottom_panel_toggle_->setChecked(true);
    view_menu->addAction(bottom_panel_toggle_);

    auto * toolbar = new QToolBar(this);
    toolbar->setMovable(false);

    search_label_ = new QLabel("Search:", this);
    search_label_->setStyleSheet("QLabel:disabled { color: rgb(180,180,180); }");
    toolbar->addWidget(search_label_);
    search_field_ = new QLineEdit(this);
    search_field_->setPlaceholderText("Search...");
    toolbar->addWidget(search_field_);

    static const QString checkbox_style =
        "QCheckBox:disabled { color: rgb(180,180,180); }";

    case_sensitive_check_ = new QCheckBox("Case", this);
    case_sensitive_check_->setLayoutDirection(Qt::RightToLeft);
    case_sensitive_check_->setStyleSheet(checkbox_style);
    toolbar->addWidget(case_sensitive_check_);

    regex_check_ = new QCheckBox("Regex", this);
    regex_check_->setLayoutDirection(Qt::RightToLeft);
    regex_check_->setStyleSheet(checkbox_style);
    toolbar->addWidget(regex_check_);

    search_col_key_ = new QCheckBox("Key", this);
    search_col_key_->setChecked(true);
    search_col_key_->setLayoutDirection(Qt::RightToLeft);
    search_col_key_->setStyleSheet(checkbox_style);
    toolbar->addWidget(search_col_key_);

    search_col_original_ = new QCheckBox("Original", this);
    search_col_original_->setChecked(true);
    search_col_original_->setLayoutDirection(Qt::RightToLeft);
    search_col_original_->setStyleSheet(checkbox_style);
    toolbar->addWidget(search_col_original_);

    search_col_translation_ = new QCheckBox("Translation", this);
    search_col_translation_->setChecked(true);
    search_col_translation_->setLayoutDirection(Qt::RightToLeft);
    search_col_translation_->setStyleSheet(checkbox_style);
    toolbar->addWidget(search_col_translation_);

    toolbar->addSeparator();

    toolbar->addWidget(new QLabel("Encoding:", this));
    encoding_combo_ = new QComboBox(this);
    encoding_combo_->addItem("Windows-1250");
    encoding_combo_->addItem("Windows-1251");
    encoding_combo_->addItem("Windows-1252");
    toolbar->addWidget(encoding_combo_);

    toolbar->addSeparator();

    toolbar->addWidget(new QLabel("Spelling:", this));
    spell_lang_combo_ = new QComboBox(this);
    spell_lang_combo_->addItem("None");
    toolbar->addWidget(spell_lang_combo_);

    grammar_check_ = new QCheckBox("Grammar", this);
    grammar_check_->setChecked(true);
    grammar_check_->setLayoutDirection(Qt::RightToLeft);
    grammar_check_->setStyleSheet(checkbox_style);
    toolbar->addWidget(grammar_check_);

    whitespace_check_ = new QCheckBox("Whitespace", this);
    whitespace_check_->setLayoutDirection(Qt::RightToLeft);
    whitespace_check_->setStyleSheet(checkbox_style);
    toolbar->addWidget(whitespace_check_);

    search_field_->setToolTip("Search across entries");
    case_sensitive_check_->setToolTip("Case-sensitive search");
    encoding_combo_->setToolTip("Text encoding");
    spell_lang_combo_->setToolTip("Spell check language");

    addToolBar(toolbar);

    find_action_ = new QAction(this);
    find_action_->setShortcut(QKeySequence("Ctrl+F"));
    addAction(find_action_);

    next_search_action_ = new QAction(this);
    next_search_action_->setShortcut(QKeySequence("F3"));
    addAction(next_search_action_);

    prev_search_action_ = new QAction(this);
    prev_search_action_->setShortcut(QKeySequence("Shift+F3"));
    addAction(prev_search_action_);

    refresh_action_ = new QAction(this);
    refresh_action_->setShortcut(QKeySequence("F5"));
    addAction(refresh_action_);

    escape_action_ = new QAction(this);
    escape_action_->setShortcut(QKeySequence("Escape"));
    addAction(escape_action_);

    find_replace_action_ = new QAction(this);
    find_replace_action_->setShortcut(QKeySequence("Ctrl+H"));
    addAction(find_replace_action_);

    auto * central_widget = new QWidget(this);
    auto * central_layout = new QVBoxLayout(central_widget);
    central_layout->setContentsMargins(0, 0, 0, 0);
    central_layout->setSpacing(4);

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
    info_tabs_->addTab(annotations_panel_, "Annotations");
    info_tabs_->addTab(history_panel_, "History");
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
    central_splitter_->setSizes({250, 1030});

    hl_original_ = new composite_highlighter_t(editor_panel_->original_view()->document());
    hl_adapted_ = new composite_highlighter_t(editor_panel_->adapted_from_view()->document());
    hl_translation_ = new composite_highlighter_t(editor_panel_->translation_editor()->document());
    hl_translation_->set_translation_mode(true);

    spell_menu_ = new spell_context_menu_t(&spell_checker_, hl_translation_);

    connect(sidebar_toggle_, &QAction::toggled, left_splitter_, &QWidget::setVisible);
    connect(bottom_panel_toggle_, &QAction::toggled, editor_panel_, &QWidget::setVisible);
    connect(whitespace_check_, &QCheckBox::toggled, this, &main_window_t::on_whitespace_toggled);
    connect(grammar_check_, &QCheckBox::toggled, this, [this]() {
        if (current_row_ < 0)
            return;

        extra_sel_translation_.grammar = grammar_check_->isChecked()
            ? grammar_checker_.check(editor_panel_->translation_editor())
            : QList<QTextEdit::ExtraSelection>{};
        apply_extra_selections(editor_panel_->translation_editor(), extra_sel_translation_);
    });

    connect(add_folder_action_, &QAction::triggered, this, [this]() {
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

    connect(import_archive_action_, &QAction::triggered, this, [this]() {
        const auto archive_path = QFileDialog::getOpenFileName(
            this, "Import Archive", "", "Archives (*.zip *.rar)");

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
        proc.start(sevenzip, {"x", archive_path, "-o" + target_dir, "-y"});
        proc.waitForFinished(60000);

        if (proc.exitCode() != 0)
        {
            QMessageBox::critical(this, "Extraction Error",
                "Failed to extract archive:\n" + proc.readAllStandardError());
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
    connect(next_search_action_, &QAction::triggered, this, &main_window_t::on_next_search);
    connect(prev_search_action_, &QAction::triggered, this, &main_window_t::on_prev_search);
    connect(refresh_action_, &QAction::triggered, this, &main_window_t::on_refresh);
    connect(escape_action_, &QAction::triggered, this, &main_window_t::on_escape);

    connect(find_replace_action_, &QAction::triggered, this, [this]() {
        if (!find_replace_dialog_)
        {
            find_replace_dialog_ = new find_replace_dialog_t(this);

            connect(find_replace_dialog_, &find_replace_dialog_t::find_next_requested, this,
                [this](const QString & query, bool case_sensitive, bool regex_mode) {
                    if (query.isEmpty())
                        return;

                    if (!table_model_)
                        return;

                    const int count = table_model_->rowCount();
                    if (count == 0)
                        return;

                    std::optional<std::regex> rx;
                    if (regex_mode)
                    {
                        auto flags = std::regex_constants::ECMAScript;
                        if (!case_sensitive)
                            flags |= std::regex_constants::icase;

                        try { rx.emplace(query.toStdString(), flags); }
                        catch (...) { return; }
                    }

                    const auto q = case_sensitive ? query : query.toLower();

                    for (int i = 1; i <= count; ++i)
                    {
                        const int row = (current_row_ + i) % count;
                        const auto * data = table_model_->row_at(row);
                        if (!data)
                            continue;

                        const auto & text = data->new_text;
                        bool found = false;

                        if (rx)
                        {
                            found = std::regex_search(text, *rx);
                        }
                        else
                        {
                            auto haystack = QString::fromStdString(text);
                            if (!case_sensitive)
                                haystack = haystack.toLower();

                            found = haystack.contains(q);
                        }

                        if (found)
                        {
                            on_row_selected(row);
                            return;
                        }
                    }

                    statusBar()->showMessage("No match found", 3000);
                });

            connect(find_replace_dialog_, &find_replace_dialog_t::replace_requested, this,
                [this](const QString & query, const QString & replacement, bool case_sensitive, bool regex_mode) {
                    if (query.isEmpty())
                        return;

                    if (current_row_ < 0)
                        return;

                    auto * slot = workspace_.get_active_slot();
                    if (!slot)
                        return;

                    const auto * row_data = table_model_->row_at(current_row_);
                    if (!row_data)
                        return;

                    auto it = slot->data.find(row_data->type);
                    if (it == slot->data.end())
                        return;

                    if (row_data->chapter_index >= it->second.records.size())
                        return;

                    auto & entry = it->second.records[row_data->chapter_index];
                    std::string result;

                    if (regex_mode)
                    {
                        auto flags = std::regex_constants::ECMAScript;
                        if (!case_sensitive)
                            flags |= std::regex_constants::icase;

                        try
                        {
                            std::regex rx(query.toStdString(), flags);
                            result = std::regex_replace(entry.new_text, rx, replacement.toStdString());
                        }
                        catch (...) { return; }
                    }
                    else
                    {
                        auto q_new = QString::fromStdString(entry.new_text);
                        auto idx = q_new.indexOf(query, 0, case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                        if (idx < 0)
                            return;

                        q_new.replace(idx, query.length(), replacement);
                        result = q_new.toStdString();
                    }

                    if (result == entry.new_text)
                        return;

                    entry.new_text = result;
                    entry.status = "in_progress";
                    slot->dirty = true;
                    slot->modified_records.insert({row_data->type, row_data->chapter_index});
                    set_dirty(true);

                    table_model_->update_row(current_row_, entry.new_text, entry.status);
                    load_record(current_row_);

                    emit find_replace_dialog_->find_next_requested(query, case_sensitive, regex_mode);
                });

            connect(find_replace_dialog_, &find_replace_dialog_t::replace_all_requested, this,
                [this](const QString & query, const QString & replacement, bool case_sensitive, bool regex_mode) {
                    if (query.isEmpty())
                        return;

                    auto * slot = workspace_.get_active_slot();
                    if (!slot)
                        return;

                    int replaced_count = 0;

                    std::optional<std::regex> rx;
                    if (regex_mode)
                    {
                        auto flags = std::regex_constants::ECMAScript;
                        if (!case_sensitive)
                            flags |= std::regex_constants::icase;

                        try { rx.emplace(query.toStdString(), flags); }
                        catch (...) { return; }
                    }

                    for (auto & [type, chapter] : slot->data)
                    {
                        for (size_t idx = 0; idx < chapter.records.size(); ++idx)
                        {
                            auto & entry = chapter.records[idx];
                            std::string result;

                            if (rx)
                            {
                                result = std::regex_replace(entry.new_text, *rx, replacement.toStdString());
                            }
                            else
                            {
                                auto q_text = QString::fromStdString(entry.new_text);
                                auto q_query = query;
                                int pos = 0;
                                bool changed = false;

                                while (true)
                                {
                                    int found = q_text.indexOf(q_query, pos, case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                                    if (found < 0)
                                        break;

                                    q_text.replace(found, q_query.length(), replacement);
                                    pos = found + replacement.length();
                                    changed = true;
                                }

                                if (!changed)
                                    continue;

                                result = q_text.toStdString();
                            }

                            if (result == entry.new_text)
                                continue;

                            entry.new_text = result;
                            entry.status = "in_progress";
                            slot->dirty = true;
                            slot->modified_records.insert({type, idx});
                            ++replaced_count;
                        }
                    }

                    if (replaced_count > 0)
                    {
                        set_dirty(true);
                        rebuild_table();
                        if (current_row_ >= 0)
                            load_record(current_row_);
                    }

                    statusBar()->showMessage(QString("Replaced in %1 entries").arg(replaced_count), 5000);
                });
        }

        find_replace_dialog_->show();
        find_replace_dialog_->raise();
        find_replace_dialog_->activateWindow();
    });

    connect(search_field_, &QLineEdit::textChanged, this, &main_window_t::on_search_changed);
    connect(case_sensitive_check_, &QCheckBox::checkStateChanged, this, [this]() { on_case_sensitive_changed(0); });
    connect(regex_check_, &QCheckBox::checkStateChanged, this, [this]() { on_search_changed(search_query_); });

    auto on_search_col_changed = [this]() { on_search_changed(search_query_); };
    connect(search_col_key_, &QCheckBox::checkStateChanged, this, on_search_col_changed);
    connect(search_col_original_, &QCheckBox::checkStateChanged, this, on_search_col_changed);
    connect(search_col_translation_, &QCheckBox::checkStateChanged, this, on_search_col_changed);

    connect(encoding_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &main_window_t::on_encoding_changed);
    connect(spell_lang_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &main_window_t::on_spell_lang_changed);

    connect(sidebar_, &sidebar_widget_t::item_clicked, this, &main_window_t::on_item_clicked);
    connect(sidebar_, &sidebar_widget_t::operation_requested, this, &main_window_t::on_operation_requested);
    connect(sidebar_, &sidebar_widget_t::save_requested, this, &main_window_t::on_save_requested);
    connect(sidebar_, &sidebar_widget_t::save_as_requested, this, [this](const std::string & path) {
        commit_current_edit();

        if (lua_active_path_.empty())
            return;

        auto sep = path.find_last_of("/\\");
        auto default_dir = sep != std::string::npos ? path.substr(0, sep) : std::string{};

        auto save_path = QFileDialog::getSaveFileName(
            this, "Save Translated YAML",
            QString::fromStdString(default_dir),
            "YAML files (*.yaml)");

        if (save_path.isEmpty())
            return;

        std::vector<std::string> key_order;
        for (const auto & e : lua_entries_)
            key_order.push_back(e.key);

        yaml_l10n_writer_t writer;
        if (!writer.write(save_path.toStdString(), lua_entries_, key_order))
            return;

        log_tab_->append_log("save as", "saved \"" + save_path.toStdString() + "\"\r\n");

        const auto tmp_path = lua_active_path_ + ".tmp";
        QFile::remove(QString::fromStdString(tmp_path));
        lua_modified_indices_.clear();

        auto * fe = file_list_.get(lua_active_path_);
        if (fe)
        {
            fe->dirty = false;
            fe->has_tmp = false;
            sidebar_->update_item_text(fe->path, derive_display_name(*fe));
        }

        lua_active_path_.clear();
        lua_entries_.clear();
        set_dirty(false);
    });
    connect(sidebar_, &sidebar_widget_t::unload_requested, this, &main_window_t::on_unload_requested);
    connect(sidebar_, &sidebar_widget_t::delete_requested, this, &main_window_t::on_delete_requested);
    connect(sidebar_, &sidebar_widget_t::remove_folder_requested, this, [this](const std::string & root_path) {
        auto & roots = config_.workspace_roots;
        roots.erase(std::remove(roots.begin(), roots.end(), root_path), roots.end());

        for (int i = workspace_.slot_count() - 1; i >= 0; --i)
        {
            const auto * slot = workspace_.get_slot(i);
            if (!slot)
                continue;

            const auto * entry = file_list_.get(slot->path);
            if (entry && entry->root_path == root_path)
                workspace_.unload_dict(i);
        }

        scan_workspace();
        save_config();
        update_watcher_paths();
    });

    connect(sidebar_, &sidebar_widget_t::delete_folder_requested, this, [this](const std::string & folder_path) {
        auto sep = folder_path.find_last_of("/\\");
        auto folder_name = sep != std::string::npos ? folder_path.substr(sep + 1) : folder_path;

        auto answer = QMessageBox::question(
            this, "Delete Folder",
            QString("Delete \"%1\" and all its contents from disk?").arg(QString::fromStdString(folder_name)),
            QMessageBox::Yes | QMessageBox::No);

        if (answer != QMessageBox::Yes)
            return;

        for (int i = workspace_.slot_count() - 1; i >= 0; --i)
        {
            const auto * slot = workspace_.get_slot(i);
            if (!slot)
                continue;

            auto normalized = slot->path;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');

            auto folder_norm = folder_path;
            std::replace(folder_norm.begin(), folder_norm.end(), '\\', '/');

            if (normalized.find(folder_norm + "/") == 0 || normalized == folder_norm)
                workspace_.unload_dict(i);
        }

        QDir(QString::fromStdString(folder_path)).removeRecursively();
        scan_workspace();
    });

    connect(table_view_, &record_table_view_t::row_selected, this, &main_window_t::on_row_selected);
    connect(table_view_, &record_table_view_t::batch_status_change_requested, this, [this](const QList<int> & rows, const QString & new_status) {
        auto * slot = workspace_.get_active_slot();
        if (!slot)
            return;

        for (int row : rows)
        {
            auto * row_data = table_model_->row_at(row);
            if (!row_data)
                continue;

            auto it = slot->data.find(row_data->type);
            if (it == slot->data.end())
                continue;

            if (row_data->chapter_index >= it->second.records.size())
                continue;

            it->second.records[row_data->chapter_index].status = new_status.toStdString();
            table_model_->update_row(row, row_data->new_text, new_status.toStdString());
        }

        slot->dirty = true;
        set_dirty(true);
        update_status_counts();
    });

    connect(editor_panel_, &editor_panel_t::text_changed, this, &main_window_t::on_translation_changed);
    connect(editor_panel_, &editor_panel_t::apply_clicked, this, [this]() {
        if (current_row_ < 0)
            return;

        commit_current_edit();

        int row_count = table_model_->rowCount();
        int next_row = -1;
        for (int i = current_row_ + 1; i < row_count; ++i)
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
            next_row = current_row_ + 1;
            if (next_row >= row_count)
                next_row = row_count - 1;
        }

        if (next_row >= 0 && next_row != current_row_)
        {
            auto idx = table_model_->index(next_row, 0);
            table_view_->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            on_row_selected(next_row);
            auto cursor = editor_panel_->translation_editor()->textCursor();
            cursor.movePosition(QTextCursor::End);
            editor_panel_->translation_editor()->setTextCursor(cursor);
            editor_panel_->translation_editor()->setFocus();
        }
    });

    connect(editor_panel_->translation_editor(), &editor_text_edit_t::navigate_next, this, [this]() {
        if (current_row_ < 0)
            return;

        commit_current_edit();

        int row_count = table_model_->rowCount();
        int next_row = -1;
        for (int i = current_row_ + 1; i < row_count; ++i)
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
            next_row = current_row_ + 1;
            if (next_row >= row_count)
                next_row = row_count - 1;
        }

        if (next_row >= 0 && next_row != current_row_)
        {
            auto idx = table_model_->index(next_row, 0);
            table_view_->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            on_row_selected(next_row);
            auto cursor = editor_panel_->translation_editor()->textCursor();
            cursor.movePosition(QTextCursor::End);
            editor_panel_->translation_editor()->setTextCursor(cursor);
            editor_panel_->translation_editor()->setFocus();
        }
    });

    connect(editor_panel_->translation_editor(), &editor_text_edit_t::navigate_prev, this, [this]() {
        if (current_row_ <= 0)
            return;

        commit_current_edit();

        int prev_row = current_row_ - 1;
        auto idx = table_model_->index(prev_row, 0);
        table_view_->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        on_row_selected(prev_row);
        auto cursor = editor_panel_->translation_editor()->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor_panel_->translation_editor()->setTextCursor(cursor);
        editor_panel_->translation_editor()->setFocus();
    });

    connect(filter_tree_, &filter_tree_t::filters_changed, this, &main_window_t::on_filters_changed);
    connect(filter_tree_, &filter_tree_t::all_reset_requested, this, [this]() {
        status_filter_.clear();
        status_filter_bar_->set_filter_state(status_filter_);
    });
    connect(status_filter_bar_, &status_filter_bar_t::filters_changed, this, &main_window_t::on_status_filters_changed);

    connect(history_panel_, &history_panel_t::revert_requested, this, [this](size_t history_index) {
        if (current_row_ < 0)
            return;

        const auto * row_data = table_model_->row_at(current_row_);
        if (!row_data)
            return;

        auto * slot = workspace_.get_active_slot();
        if (!slot)
            return;

        history_manager_.revert(*slot, row_data->type, row_data->key_text, history_index);
        slot->dirty = true;
        set_dirty(true);

        const auto * updated_row = table_model_->row_at(current_row_);
        if (updated_row)
        {
            auto type_it = slot->data.find(updated_row->type);
            if (type_it != slot->data.end() && updated_row->chapter_index < type_it->second.records.size())
            {
                const auto & rec = type_it->second.records[updated_row->chapter_index];
                table_model_->update_row(current_row_, rec.new_text, rec.status);
            }
        }

        load_record(current_row_);
    });

    fs_watcher_ = new QFileSystemWatcher(this);
    rescan_timer_ = new QTimer(this);
    rescan_timer_->setSingleShot(true);
    rescan_timer_->setInterval(200);
    connect(rescan_timer_, &QTimer::timeout, this, [this]() {
        scan_workspace();

        const auto * slot = workspace_.get_active_slot();
        if (slot && !QFile::exists(QString::fromStdString(slot->path)))
        {
            const int active = workspace_.get_active_index();
            workspace_.unload_dict(active);
            rebuild_table();
        }
    });
    connect(fs_watcher_, &QFileSystemWatcher::directoryChanged,
            rescan_timer_, qOverload<>(&QTimer::start));

    scan_spell_dictionaries();

    const auto config_path = QCoreApplication::applicationDirPath() + "/yampt_gui.ini";
    bool first_run = !QFile::exists(config_path);

    load_config();

    if (first_run)
    {
        std::vector<std::string> spell_langs;
        for (int i = 1; i < spell_lang_combo_->count(); ++i)
            spell_langs.push_back(spell_lang_combo_->itemText(i).toStdString());

        first_run_dialog_t dialog(spell_langs, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            encoding_combo_->setCurrentIndex(dialog.selected_encoding_index());
            spell_lang_combo_->setCurrentIndex(dialog.selected_spell_lang_index());
            save_config();
        }
    }
}

void main_window_t::set_dirty(bool dirty)
{
    if (dirty_ == dirty)
        return;

    dirty_ = dirty;
    setWindowTitle(dirty_ ? "yampt.gui *" : "yampt.gui");
}

void main_window_t::on_save()
{
    commit_current_edit();

    if (!lua_active_path_.empty())
    {
        save_lua_temp();

        auto * fe = file_list_.get(lua_active_path_);
        if (fe)
        {
            fe->dirty = false;
            sidebar_->update_item_text(fe->path, derive_display_name(*fe));
        }

        set_dirty(false);
        return;
    }

    int active = workspace_.get_active_index();
    if (active < 0)
        return;

    if (!workspace_.is_user_slot(active))
        return;

    const auto * slot = workspace_.get_slot(active);
    save_dict_encoded(active);

    if (slot)
    {
        auto * fe = file_list_.get(slot->path);
        if (fe)
            sidebar_->update_item_text(fe->path, derive_display_name(*fe));

        log_tab_->append_log("save", "saved \"" + slot->path + "\"\r\n");
    }

    if (!workspace_.has_any_unsaved())
        set_dirty(false);
}

void main_window_t::on_save_all()
{
    commit_current_edit();

    std::string log_msg;
    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot && slot->dirty)
        {
            save_dict_encoded(i);

            auto * fe = file_list_.get(slot->path);
            if (fe)
                sidebar_->update_item_text(fe->path, derive_display_name(*fe));

            log_msg += "saved \"" + slot->path + "\"\r\n";
        }
    }

    if (!log_msg.empty())
        log_tab_->append_log("save all", log_msg);

    if (!workspace_.has_any_unsaved())
        set_dirty(false);
}

void main_window_t::on_unload_slot(int index)
{
    auto * slot = workspace_.get_slot(index);
    if (!slot)
        return;

    if (slot->dirty)
    {
        auto answer = QMessageBox::question(
            this, "Unsaved Changes",
            "This dictionary has unsaved changes. Save before unloading?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (answer == QMessageBox::Cancel)
            return;

        if (answer == QMessageBox::Save)
            save_dict_encoded(index);
    }

    workspace_.unload_dict(index);

    std::vector<dict_source_t> sources;
    for (const auto & dict_slot_ref : workspace_.get_all_slots())
        sources.push_back({&dict_slot_ref.data, dict_slot_ref.path});
    annotation_manager_.rebuild(sources);

    rebuild_sidebar();
    rebuild_table();
}

void main_window_t::on_find()
{
    search_field_->setFocus();
    search_field_->selectAll();
}

void main_window_t::on_next_search()
{
    if (search_query_.isEmpty())
        return;

    if (!table_model_)
        return;

    const int count = table_model_->rowCount();
    if (count == 0)
        return;

    const bool case_sensitive = case_sensitive_check_->isChecked();
    const auto query = case_sensitive ? search_query_ : search_query_.toLower();

    for (int i = 1; i <= count; ++i)
    {
        const int row = (current_row_ + i) % count;
        const auto * data = table_model_->row_at(row);
        if (!data)
            continue;

        auto key = QString::fromStdString(data->key_text);
        auto old_t = QString::fromStdString(data->old_text);
        auto new_t = QString::fromStdString(data->new_text);

        if (!case_sensitive)
        {
            key = key.toLower();
            old_t = old_t.toLower();
            new_t = new_t.toLower();
        }

        if (key.contains(query) || old_t.contains(query) || new_t.contains(query))
        {
            on_row_selected(row);
            return;
        }
    }
}

void main_window_t::on_prev_search()
{
    if (search_query_.isEmpty())
        return;

    if (!table_model_)
        return;

    const int count = table_model_->rowCount();
    if (count == 0)
        return;

    const bool case_sensitive = case_sensitive_check_->isChecked();
    const auto query = case_sensitive ? search_query_ : search_query_.toLower();

    for (int i = 1; i <= count; ++i)
    {
        const int row = (current_row_ - i + count) % count;
        const auto * data = table_model_->row_at(row);
        if (!data)
            continue;

        auto key = QString::fromStdString(data->key_text);
        auto old_t = QString::fromStdString(data->old_text);
        auto new_t = QString::fromStdString(data->new_text);

        if (!case_sensitive)
        {
            key = key.toLower();
            old_t = old_t.toLower();
            new_t = new_t.toLower();
        }

        if (key.contains(query) || old_t.contains(query) || new_t.contains(query))
        {
            on_row_selected(row);
            return;
        }
    }
}

void main_window_t::on_refresh()
{
    if (current_row_ < 0)
        return;

    load_record(current_row_);
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
    type_filter_solo_ = false;
    rebuild_table();
}

void main_window_t::on_status_filters_changed()
{
    status_filter_ = status_filter_bar_->get_active_statuses();
    rebuild_table();
}

static std::string extract_info_prefix(const std::string & key_text)
{
    size_t first = key_text.find('^');
    if (first == std::string::npos)
        return {};

    size_t second = key_text.find('^', first + 1);
    if (second == std::string::npos)
        return {};

    size_t third = key_text.find('^', second + 1);
    if (third == std::string::npos)
        return key_text;

    return key_text.substr(0, third);
}

void main_window_t::rebuild_table()
{
    if (!table_model_)
        return;

    if (!lua_active_path_.empty())
        return;

    const auto * slot = workspace_.get_active_slot();
    if (!slot)
    {
        table_model_->rebuild({});
        progress_label_->clear();
        active_file_label_->clear();
        filter_tree_->setEnabled(false);
        status_filter_bar_->set_dict_mode(status_filter_bar_t::dict_mode_t::none);
        search_label_->setEnabled(false);
        search_field_->setEnabled(false);
        case_sensitive_check_->setEnabled(false);
        regex_check_->setEnabled(false);
        search_col_key_->setEnabled(false);
        search_col_original_->setEnabled(false);
        search_col_translation_->setEnabled(false);
        return;
    }

    int active_idx = workspace_.get_active_index();
    if (active_idx >= 0 && workspace_.is_base_slot(active_idx))
        status_filter_bar_->set_dict_mode(status_filter_bar_t::dict_mode_t::base);
    else
        status_filter_bar_->set_dict_mode(status_filter_bar_t::dict_mode_t::user);

    filter_tree_->set_display_mode(filter_tree_t::display_mode_t::full);
    filter_tree_->setEnabled(true);
    active_file_label_->setText(QString::fromStdString(slot->path));
    search_label_->setEnabled(true);
    search_field_->setEnabled(true);
    case_sensitive_check_->setEnabled(true);
    regex_check_->setEnabled(true);
    search_col_key_->setEnabled(true);
    search_col_original_->setEnabled(true);
    search_col_translation_->setEnabled(true);

    const auto active_sub_types = filter_tree_->get_active_sub_types();
    const size_t total_sub_types = 5 + 23 + 3 + 2;
    const bool has_sub_type_filter = active_sub_types.size() < total_sub_types;

    static const std::map<std::string, std::string> sub_type_to_prefix = {
        {"Topic", "T"}, {"Voice", "V"}, {"Greeting", "G"}, {"Persuasion", "P"}, {"Journal", "J"},
        {"ACTI", "ACTI"}, {"ALCH", "ALCH"}, {"APPA", "APPA"}, {"ARMO", "ARMO"}, {"BOOK", "BOOK"},
        {"BSGN", "BSGN"}, {"CLAS", "CLAS"}, {"CLOT", "CLOT"}, {"CONT", "CONT"}, {"CREA", "CREA"},
        {"DOOR", "DOOR"}, {"FACT", "FACT"}, {"INGR", "INGR"}, {"LIGH", "LIGH"}, {"LOCK", "LOCK"},
        {"MISC", "MISC"}, {"NPC_", "NPC_"}, {"PROB", "PROB"}, {"RACE", "RACE"}, {"REGN", "REGN"},
        {"REPA", "REPA"}, {"SPEL", "SPEL"}, {"WEAP", "WEAP"},
        {"Birthsigns", "BSGN"}, {"Classes", "CLAS"}, {"Races", "RACE"},
        {"Skills", "SKIL"}, {"Magic Effects", "MGEF"},
    };

    static const std::map<std::pair<tools_t::rec_type_t, std::string>, std::string> prefix_to_sub_type = {
        {{tools_t::rec_type_t::info, "T"}, "Topic"},
        {{tools_t::rec_type_t::info, "V"}, "Voice"},
        {{tools_t::rec_type_t::info, "G"}, "Greeting"},
        {{tools_t::rec_type_t::info, "P"}, "Persuasion"},
        {{tools_t::rec_type_t::info, "J"}, "Journal"},
        {{tools_t::rec_type_t::bnam, "T"}, "Topic"},
        {{tools_t::rec_type_t::bnam, "V"}, "Voice"},
        {{tools_t::rec_type_t::bnam, "G"}, "Greeting"},
        {{tools_t::rec_type_t::bnam, "P"}, "Persuasion"},
        {{tools_t::rec_type_t::bnam, "J"}, "Journal"},
        {{tools_t::rec_type_t::fnam, "ACTI"}, "ACTI"}, {{tools_t::rec_type_t::fnam, "ALCH"}, "ALCH"},
        {{tools_t::rec_type_t::fnam, "APPA"}, "APPA"}, {{tools_t::rec_type_t::fnam, "ARMO"}, "ARMO"},
        {{tools_t::rec_type_t::fnam, "BOOK"}, "BOOK"}, {{tools_t::rec_type_t::fnam, "BSGN"}, "BSGN"},
        {{tools_t::rec_type_t::fnam, "CLAS"}, "CLAS"}, {{tools_t::rec_type_t::fnam, "CLOT"}, "CLOT"},
        {{tools_t::rec_type_t::fnam, "CONT"}, "CONT"}, {{tools_t::rec_type_t::fnam, "CREA"}, "CREA"},
        {{tools_t::rec_type_t::fnam, "DOOR"}, "DOOR"}, {{tools_t::rec_type_t::fnam, "FACT"}, "FACT"},
        {{tools_t::rec_type_t::fnam, "INGR"}, "INGR"}, {{tools_t::rec_type_t::fnam, "LIGH"}, "LIGH"},
        {{tools_t::rec_type_t::fnam, "LOCK"}, "LOCK"}, {{tools_t::rec_type_t::fnam, "MISC"}, "MISC"},
        {{tools_t::rec_type_t::fnam, "NPC_"}, "NPC_"}, {{tools_t::rec_type_t::fnam, "PROB"}, "PROB"},
        {{tools_t::rec_type_t::fnam, "RACE"}, "RACE"}, {{tools_t::rec_type_t::fnam, "REGN"}, "REGN"},
        {{tools_t::rec_type_t::fnam, "REPA"}, "REPA"}, {{tools_t::rec_type_t::fnam, "SPEL"}, "SPEL"},
        {{tools_t::rec_type_t::fnam, "WEAP"}, "WEAP"},
        {{tools_t::rec_type_t::desc, "BSGN"}, "Birthsigns"},
        {{tools_t::rec_type_t::desc, "CLAS"}, "Classes"},
        {{tools_t::rec_type_t::desc, "RACE"}, "Races"},
        {{tools_t::rec_type_t::indx, "SKIL"}, "Skills"},
        {{tools_t::rec_type_t::indx, "MGEF"}, "Magic Effects"},
    };

    static const std::set<std::string> done_statuses_user = {
        "translated"
    };

    static const std::set<std::string> done_statuses_base = {
        "matched", "fingerprint", "coords", "heuristic", "exact",
        "info", "wilderness", "region"
    };

    const bool is_base = workspace_.is_base_slot(workspace_.get_active_index());
    const auto & done_statuses = is_base ? done_statuses_base : done_statuses_user;

    std::vector<table_row_t> rows;
    std::map<tools_t::rec_type_t, size_t> type_counts;
    std::map<tools_t::rec_type_t, size_t> translated_counts;
    std::map<std::string, size_t> status_counts;
    std::map<std::string, size_t> filtered_status_counts;
    std::map<std::string, size_t> sub_type_counts;
    std::map<std::string, size_t> sub_type_translated_counts;

    std::unordered_multimap<std::string, size_t> bnam_prefix_map;
    std::set<size_t> consumed_bnams;

    auto bnam_it = slot->data.find(tools_t::rec_type_t::bnam);
    if (bnam_it != slot->data.end())
    {
        for (size_t i = 0; i < bnam_it->second.records.size(); ++i)
        {
            const auto & entry = bnam_it->second.records[i];
            auto prefix = extract_info_prefix(entry.key_text);
            if (!prefix.empty())
                bnam_prefix_map.emplace(prefix, i);
        }
    }

    const bool info_in_filter = type_filter_.empty() || type_filter_.count(tools_t::rec_type_t::info) > 0;

    for (const auto & [type, chapter] : slot->data)
    {
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            const auto & entry = chapter.records[i];
            type_counts[type]++;
            status_counts[entry.status]++;

            if (type == tools_t::rec_type_t::info || type == tools_t::rec_type_t::bnam ||
                type == tools_t::rec_type_t::fnam || type == tools_t::rec_type_t::desc ||
                type == tools_t::rec_type_t::indx)
            {
                auto caret_pos = entry.key_text.find('^');
                if (caret_pos != std::string::npos && caret_pos > 0)
                {
                    auto prefix = entry.key_text.substr(0, caret_pos);
                    auto p2s_it = prefix_to_sub_type.find({type, prefix});
                    if (p2s_it != prefix_to_sub_type.end())
                    {
                        sub_type_counts[p2s_it->second]++;
                        if (done_statuses.count(entry.status))
                            sub_type_translated_counts[p2s_it->second]++;
                    }
                }
            }

            if (done_statuses.count(entry.status))
                translated_counts[type]++;

            if (type == tools_t::rec_type_t::bnam && consumed_bnams.count(i) > 0)
                continue;

            if (!type_filter_.empty() && type_filter_.count(type) == 0)
                continue;

            if (has_sub_type_filter && (type == tools_t::rec_type_t::info || type == tools_t::rec_type_t::bnam ||
                type == tools_t::rec_type_t::fnam || type == tools_t::rec_type_t::desc || type == tools_t::rec_type_t::indx))
            {
                bool sub_match = false;
                auto caret_pos = entry.key_text.find('^');
                if (caret_pos != std::string::npos && caret_pos > 0)
                {
                    auto prefix = entry.key_text.substr(0, caret_pos);
                    for (const auto & sub : active_sub_types)
                    {
                        auto it = sub_type_to_prefix.find(sub);
                        if (it != sub_type_to_prefix.end() && it->second == prefix)
                        {
                            sub_match = true;
                            break;
                        }
                    }
                }

                if (!sub_match)
                    continue;
            }

            {
                table_row_t tmp_row;
                tmp_row.type = type;
                tmp_row.key_text = entry.key_text;
                tmp_row.old_text = entry.old_text;
                tmp_row.new_text = entry.new_text;
                tmp_row.status = entry.status;
                tmp_row.chapter_index = i;

                if (!search_engine_.has_query() || search_engine_.matches(tmp_row))
                    filtered_status_counts[entry.status]++;
            }

            if (!status_filter_.empty() && status_filter_.count(entry.status) == 0)
                continue;

            table_row_t row;
            row.type = type;
            row.key_text = entry.key_text;
            row.old_text = entry.old_text;
            row.new_text = entry.new_text;
            row.status = entry.status;
            row.chapter_index = i;

            if (search_engine_.has_query() && !search_engine_.matches(row))
                continue;

            rows.push_back(std::move(row));

            if (type == tools_t::rec_type_t::info && info_in_filter && bnam_it != slot->data.end())
            {
                auto info_prefix = extract_info_prefix(entry.key_text);
                if (info_prefix.empty())
                    continue;

                auto [begin, end] = bnam_prefix_map.equal_range(info_prefix);
                for (auto it = begin; it != end; ++it)
                {
                    const auto & bnam_entry = bnam_it->second.records[it->second];

                    {
                        table_row_t tmp_child;
                        tmp_child.type = tools_t::rec_type_t::bnam;
                        tmp_child.key_text = bnam_entry.key_text;
                        tmp_child.old_text = bnam_entry.old_text;
                        tmp_child.new_text = bnam_entry.new_text;
                        tmp_child.status = bnam_entry.status;
                        tmp_child.chapter_index = it->second;
                        tmp_child.is_child = true;

                        if (!search_engine_.has_query() || search_engine_.matches(tmp_child))
                            filtered_status_counts[bnam_entry.status]++;
                    }

                    if (!status_filter_.empty() && status_filter_.count(bnam_entry.status) == 0)
                        continue;

                    table_row_t child;
                    child.type = tools_t::rec_type_t::bnam;
                    child.key_text = bnam_entry.key_text;
                    child.old_text = bnam_entry.old_text;
                    child.new_text = bnam_entry.new_text;
                    child.status = bnam_entry.status;
                    child.chapter_index = it->second;
                    child.is_child = true;

                    if (search_engine_.has_query() && !search_engine_.matches(child))
                        continue;

                    rows.push_back(std::move(child));
                    consumed_bnams.insert(it->second);
                }
            }
        }
    }

    table_model_->rebuild(std::move(rows));
    current_row_ = -1;

    std::map<std::string, size_t> total_status_counts;
    for (const auto & [type, chapter] : slot->data)
    {
        for (const auto & rec : chapter.records)
            total_status_counts[rec.status]++;
    }

    size_t total = 0;
    size_t total_translated = 0;
    for (const auto & [t, c] : type_counts)
        total += c;
    for (const auto & [t, c] : translated_counts)
        total_translated += c;

    filter_tree_->update_counts(type_counts, translated_counts);
    filter_tree_->update_sub_type_counts(sub_type_counts, sub_type_translated_counts);
    filter_tree_->set_total_count(total_translated, total);
    status_filter_bar_->update_counts(filtered_status_counts, total_status_counts);

    size_t progress_translated = 0;
    size_t progress_total = 0;
    for (const auto & [type, chapter] : slot->data)
    {
        if (!type_filter_.empty() && type_filter_.count(type) == 0)
            continue;

        for (const auto & entry : chapter.records)
        {
            progress_total++;
            if (done_statuses.count(entry.status))
                progress_translated++;
        }
    }

    if (progress_total > 0)
    {
        int pct = static_cast<int>(progress_translated * 100 / progress_total);
        int shown = table_model_->rowCount();
        progress_label_->setText(QString("%1 / %2 (%3%) | %4 shown")
            .arg(progress_translated).arg(progress_total).arg(pct).arg(shown));
    }
    else
    {
        progress_label_->clear();
    }
}

void main_window_t::on_row_selected(int row)
{
    if (row == current_row_)
        return;

    commit_current_edit();
    load_record(row);
}

void main_window_t::on_translation_changed()
{
    if (loading_record_)
        return;

    if (current_row_ < 0)
        return;

    if (!lua_active_path_.empty())
    {
        auto * fe = file_list_.get(lua_active_path_);
        if (fe && !fe->dirty)
        {
            fe->dirty = true;
            fe->has_tmp = true;
            sidebar_->update_item_text(fe->path, derive_display_name(*fe));
            set_dirty(true);
        }

        return;
    }

    if (workspace_.get_active_index() >= 0)
    {
        auto * slot = workspace_.get_slot(workspace_.get_active_index());
        if (slot && !slot->dirty)
        {
            slot->dirty = true;
            file_list_.set_dirty(slot->path, true);

            auto * fe = file_list_.get(slot->path);
            if (fe)
                sidebar_->update_item_text(fe->path, derive_display_name(*fe));
        }
    }

    set_dirty(true);

    update_validation();

    if (current_row_ < 0)
        return;

    const auto * row_data = table_model_->row_at(current_row_);
    if (!row_data)
        return;

    const auto annotations = annotation_manager_.annotate(row_data->old_text, row_data->type);
    const auto current_text = editor_panel_->translation_editor()->toPlainText().toLower();

    struct highlight_t { int start; int length; bool is_hyperlink; };
    struct candidate_t { int start; int length; bool is_hyperlink; };

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
            candidates.push_back({pos, static_cast<int>(term.length()), is_hl});
            pos += static_cast<int>(term.length());
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const candidate_t & a, const candidate_t & b)
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
        ? grammar_checker_.check(editor_panel_->translation_editor())
        : QList<QTextEdit::ExtraSelection>{};
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
                " len=" + std::to_string(end - start) +
                " text=\"" + slice.toStdString() + "\"\r\n";
        }
        log_tab_->append_log("grammar", log_msg);
    }
}

int main_window_t::propagate_translation(const std::string & old_text, const std::string & new_text)
{
    auto * slot = workspace_.get_active_slot();
    if (!slot)
        return 0;

    auto trimmed_source = old_text;
    while (!trimmed_source.empty() && (trimmed_source.front() == ' ' || trimmed_source.front() == '\t'))
        trimmed_source.erase(trimmed_source.begin());
    while (!trimmed_source.empty() && (trimmed_source.back() == ' ' || trimmed_source.back() == '\t'))
        trimmed_source.pop_back();

    int count = 0;
    for (auto & [type, chapter] : slot->data)
    {
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            auto & entry = chapter.records[i];

            auto trimmed = entry.old_text;
            while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
                trimmed.erase(trimmed.begin());
            while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
                trimmed.pop_back();

            if (trimmed != trimmed_source)
                continue;

            if (entry.new_text == new_text)
                continue;

            entry.new_text = new_text;
            entry.status = "propagated";
            ++count;
        }
    }

    return count;
}

void main_window_t::commit_current_edit()
{
    if (current_row_ < 0)
        return;

    if (loading_record_)
        return;

    if (!editor_panel_)
        return;

    const auto & current_text = editor_panel_->translation_editor()->toPlainText();
    if (current_text == loaded_text_)
        return;

    const auto * row_data = table_model_->row_at(current_row_);
    if (!row_data)
        return;

    if (row_data->type == tools_t::rec_type_t::lua)
    {
        if (row_data->chapter_index < lua_entries_.size())
        {
            lua_entries_[row_data->chapter_index].value = current_text.toStdString();
            lua_modified_indices_.insert(row_data->chapter_index);
            table_model_->update_row(current_row_, current_text.toStdString(), "in_progress");
            save_lua_temp();
        }

        loaded_text_ = current_text;
        return;
    }

    auto * slot = workspace_.get_active_slot();
    if (!slot)
        return;

    auto it = slot->data.find(row_data->type);
    if (it == slot->data.end())
        return;

    if (row_data->chapter_index >= it->second.records.size())
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
                QString("Warning: expected %1 strings, got %2")
                    .arg(slot_count)
                    .arg(lines.size()),
                5000);
        }
    }
    else
    {
        new_text_str = current_text.toStdString();
    }

    history_manager_.record_change(row_data->type, row_data->key_text, loaded_text_.toStdString(), new_text_str);

    const auto validation = validation_manager_.validate(row_data->type, new_text_str);
    const std::string commit_status = (validation.level == validation_level_t::error) ? "error" : "in_progress";

    it->second.records[row_data->chapter_index].new_text = new_text_str;
    it->second.records[row_data->chapter_index].status = commit_status;
    slot->dirty = true;
    slot->modified_records.insert({row_data->type, row_data->chapter_index});
    set_dirty(true);

    annotation_manager_.update_term(row_data->type, it->second.records[row_data->chapter_index].old_text, new_text_str);

    if (new_text_str != it->second.records[row_data->chapter_index].old_text)
    {
        int propagated = propagate_translation(it->second.records[row_data->chapter_index].old_text, new_text_str);
        if (propagated > 0)
        {
            it->second.records[row_data->chapter_index].status = "propagated";
            slot->dirty = true;
            statusBar()->showMessage(QString("Propagated to %1 entries").arg(propagated), 5000);

            table_model_->update_row(current_row_, new_text_str, "propagated");

            for (int i = 0; i < table_model_->rowCount(); ++i)
            {
                if (i == current_row_)
                    continue;

                const auto * r = table_model_->row_at(i);
                if (!r)
                    continue;

                auto chap_it = slot->data.find(r->type);
                if (chap_it == slot->data.end())
                    continue;

                if (r->chapter_index >= chap_it->second.records.size())
                    continue;

                const auto & rec = chap_it->second.records[r->chapter_index];
                if (rec.new_text != r->new_text || rec.status != r->status)
                    table_model_->update_row(i, rec.new_text, rec.status);
            }

            loaded_text_ = current_text;
            update_status_counts();
            return;
        }
    }

    table_model_->update_row(current_row_, new_text_str, commit_status);
    loaded_text_ = current_text;
    update_status_counts();
}

void main_window_t::load_record(int row)
{
    loading_record_ = true;

    if (!table_model_ || !editor_panel_)
    {
        current_row_ = -1;
        loading_record_ = false;
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
        current_row_ = -1;
        loading_record_ = false;
        return;
    }

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

        bool block = (row_data->type == tools_t::rec_type_t::cell ||
                      row_data->type == tools_t::rec_type_t::dial ||
                      row_data->type == tools_t::rec_type_t::fnam);
        editor_panel_->translation_editor()->set_block_multiline(block);
    }

    const bool is_base = !lua_active_path_.empty() ? false
        : workspace_.is_base_slot(workspace_.get_active_index());
    editor_panel_->translation_editor()->setReadOnly(is_base);

    hl_original_->set_record_type(row_data->type);
    hl_adapted_->set_record_type(row_data->type);
    hl_translation_->set_record_type(row_data->type);

    const auto validation_result = validation_manager_.validate(row_data->type, row_data->new_text);
    validation_indicator_->update_validation(validation_result);

    if (row_data->type == tools_t::rec_type_t::text)
        book_preview_->set_html(row_data->new_text);
    else
        book_preview_->clear();

    const auto annotations = annotation_manager_.annotate(row_data->old_text, row_data->type);

    std::string speaker_name;
    std::string gender_str;
    std::string enchantment_str;
    std::string adapted_from_str;

    auto * active_slot = workspace_.get_active_slot();
    if (active_slot)
    {
        auto it = active_slot->data.find(row_data->type);
        if (it != active_slot->data.end() && row_data->chapter_index < it->second.records.size())
        {
            const auto & entry = it->second.records[row_data->chapter_index];
            speaker_name = entry.speaker_name;
            gender_str = entry.gender;
            enchantment_str = entry.enchantment;
            adapted_from_str = entry.adapted_from;
        }
    }

    annotations_panel_->update_annotations(annotations, speaker_name, gender_str, enchantment_str);

    if ((row_data->status == "adapted" || row_data->status == "changed") && !adapted_from_str.empty())
    {
        editor_panel_->set_adapted_from(adapted_from_str);
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

    auto find_highlights = [](const QString & text_lower, const std::vector<annotation_t> & annotations, bool use_old) -> std::vector<highlight_t>
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
                candidates.push_back({pos, static_cast<int>(term.length()), is_hl});
                pos += static_cast<int>(term.length());
            }
        }

        std::sort(candidates.begin(), candidates.end(), [](const candidate_t & a, const candidate_t & b)
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

            results.push_back({c.start, c.length, c.is_hyperlink});
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
                " len=" + std::to_string(h.length) +
                " text=\"" + slice.toStdString() + "\"\r\n";
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
        ? grammar_checker_.check(editor_panel_->translation_editor())
        : QList<QTextEdit::ExtraSelection>{};
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
                " len=" + std::to_string(end - start) +
                " text=\"" + slice.toStdString() + "\"\r\n";
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
                " qchar_pos=" + std::to_string(qchar_start) +
                " len=" + std::to_string(qchar_len) +
                " word=\"" + m.word + "\""
                " slice=\"" + slice.toStdString() + "\"\r\n";
        }
        log_tab_->append_log("spelling", spell_msg);
    }

    extra_sel_adapted_.annotations.clear();
    extra_sel_adapted_.grammar.clear();
    extra_sel_adapted_.adapted_diff.clear();

    if (row_data->status == "adapted" && !adapted_from_str.empty())
    {
        extra_sel_adapted_.adapted_diff = editor_panel_->highlight_adapted_diff(row_data->new_text, adapted_from_str, false);
    }
    else if (row_data->status == "changed" && !adapted_from_str.empty())
    {
        extra_sel_adapted_.adapted_diff = editor_panel_->highlight_adapted_diff(row_data->old_text, adapted_from_str, true);
    }

    apply_extra_selections(editor_panel_->adapted_from_view(), extra_sel_adapted_);

    const auto history = history_manager_.get_history(row_data->type, row_data->key_text);
    history_panel_->update_history(history, !is_base);

    loaded_text_ = editor_panel_->translation_editor()->toPlainText();
    current_row_ = row;

    auto cursor = editor_panel_->translation_editor()->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor_panel_->translation_editor()->setTextCursor(cursor);

    loading_record_ = false;
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
    config_.encoding_index = index;
    save_config();

    QMessageBox::information(this, "Restart Required",
        "Encoding changed. Please restart the application for the change to take effect.");
}

void main_window_t::save_dict_encoded(int slot_index)
{
    auto * slot = workspace_.get_slot(slot_index);
    if (!slot)
        return;

    tools_t::dict_t encoded;
    for (const auto & [type, chapter] : slot->data)
    {
        for (const auto & entry : chapter.records)
        {
            tools_t::record_entry_t enc_entry = entry;
            enc_entry.key_text = encode_from_utf8(entry.key_text, current_codepage_);
            enc_entry.old_text = encode_from_utf8(entry.old_text, current_codepage_);
            enc_entry.new_text = encode_from_utf8(entry.new_text, current_codepage_);

            if (!entry.adapted_from.empty())
                enc_entry.adapted_from = encode_from_utf8(entry.adapted_from, current_codepage_);

            encoded[type].insert(enc_entry);
        }
    }

    dict_writer_t::write(encoded, slot->path);
    slot->dirty = false;
    slot->modified_records.clear();
    file_list_.set_dirty(slot->path, false);
}

void main_window_t::rebuild_sidebar()
{
	std::string active_path;
	if (!lua_active_path_.empty())
		active_path = lua_active_path_;
	else if (workspace_.get_active_index() >= 0)
		active_path = workspace_.get_slot(workspace_.get_active_index())->path;

	auto model = build_render_model(file_list_, active_path);

	static const std::set<std::string> done_statuses_user = {"translated", "in_progress", "propagated"};
	static const std::set<std::string> done_statuses_base = {
		"matched", "fingerprint", "coords", "heuristic", "exact", "info", "wilderness", "region"};

	std::function<void(sidebar_render_node_t &)> fill_counts;
	fill_counts = [&](sidebar_render_node_t & node)
	{
		for (auto & item : node.items)
		{
			if (item.type == file_type_t::plugin)
				continue;

			if (item.type == file_type_t::lua_l10n && item.path == lua_active_path_)
			{
				item.total_count = static_cast<int>(lua_entries_.size());
				item.translated_count = static_cast<int>(lua_modified_indices_.size());
				continue;
			}

			for (int i = 0; i < workspace_.slot_count(); ++i)
			{
				const auto * slot = workspace_.get_slot(i);
				if (!slot || slot->path != item.path)
					continue;

				const bool is_base = workspace_.is_base_slot(i);
				const auto & done = is_base ? done_statuses_base : done_statuses_user;

				int total = 0;
				int translated = 0;
				for (const auto & [type, chapter] : slot->data)
				{
					for (const auto & rec : chapter.records)
					{
						total++;
						if (done.count(rec.status))
							translated++;
					}
				}

				item.translated_count = translated;
				item.total_count = total;
				break;
			}
		}

		for (auto & child : node.children)
			fill_counts(child);
	};

	for (auto & root : model.roots)
		fill_counts(root);

	sidebar_->set_model(model);
}

void main_window_t::update_annotations()
{
    if (current_row_ < 0)
        return;

    const auto * row_data = table_model_->row_at(current_row_);
    if (!row_data)
        return;

    const auto annotations = annotation_manager_.annotate(row_data->old_text, row_data->type);

    std::string speaker_name;
    std::string gender_str;
    std::string enchantment_str;

    auto * active_slot = workspace_.get_active_slot();
    if (active_slot)
    {
        auto it = active_slot->data.find(row_data->type);
        if (it != active_slot->data.end() && row_data->chapter_index < it->second.records.size())
        {
            const auto & entry = it->second.records[row_data->chapter_index];
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
    const auto * slot = workspace_.get_active_slot();
    if (!slot)
        return;

    static const std::set<std::string> done_statuses_user = { "translated" };
    static const std::set<std::string> done_statuses_base = {
        "matched", "fingerprint", "coords", "heuristic", "exact",
        "info", "wilderness", "region"
    };

    const bool is_base = workspace_.is_base_slot(workspace_.get_active_index());
    const auto & done_statuses = is_base ? done_statuses_base : done_statuses_user;

    std::map<std::string, size_t> total_status_counts;
    std::map<tools_t::rec_type_t, size_t> type_counts;
    std::map<tools_t::rec_type_t, size_t> translated_counts;

    std::map<std::string, size_t> sub_type_counts_2;
    std::map<std::string, size_t> sub_type_translated_2;
    for (const auto & [type, chapter] : slot->data)
    {
        for (const auto & rec : chapter.records)
        {
            total_status_counts[rec.status]++;
            type_counts[type]++;

            if (done_statuses.count(rec.status))
                translated_counts[type]++;

            if (type == tools_t::rec_type_t::info || type == tools_t::rec_type_t::bnam ||
                type == tools_t::rec_type_t::fnam || type == tools_t::rec_type_t::desc ||
                type == tools_t::rec_type_t::indx)
            {
                auto caret_pos = rec.key_text.find('^');
                if (caret_pos != std::string::npos && caret_pos > 0)
                {
                    auto prefix = rec.key_text.substr(0, caret_pos);
                    static const std::map<std::pair<tools_t::rec_type_t, std::string>, std::string> prefix_to_sub_type = {
                        {{tools_t::rec_type_t::info, "T"}, "Topic"},
                        {{tools_t::rec_type_t::info, "V"}, "Voice"},
                        {{tools_t::rec_type_t::info, "G"}, "Greeting"},
                        {{tools_t::rec_type_t::info, "P"}, "Persuasion"},
                        {{tools_t::rec_type_t::info, "J"}, "Journal"},
                        {{tools_t::rec_type_t::bnam, "T"}, "Topic"},
                        {{tools_t::rec_type_t::bnam, "V"}, "Voice"},
                        {{tools_t::rec_type_t::bnam, "G"}, "Greeting"},
                        {{tools_t::rec_type_t::bnam, "P"}, "Persuasion"},
                        {{tools_t::rec_type_t::bnam, "J"}, "Journal"},
                        {{tools_t::rec_type_t::fnam, "ACTI"}, "ACTI"}, {{tools_t::rec_type_t::fnam, "ALCH"}, "ALCH"},
                        {{tools_t::rec_type_t::fnam, "APPA"}, "APPA"}, {{tools_t::rec_type_t::fnam, "ARMO"}, "ARMO"},
                        {{tools_t::rec_type_t::fnam, "BOOK"}, "BOOK"}, {{tools_t::rec_type_t::fnam, "BSGN"}, "BSGN"},
                        {{tools_t::rec_type_t::fnam, "CLAS"}, "CLAS"}, {{tools_t::rec_type_t::fnam, "CLOT"}, "CLOT"},
                        {{tools_t::rec_type_t::fnam, "CONT"}, "CONT"}, {{tools_t::rec_type_t::fnam, "CREA"}, "CREA"},
                        {{tools_t::rec_type_t::fnam, "DOOR"}, "DOOR"}, {{tools_t::rec_type_t::fnam, "FACT"}, "FACT"},
                        {{tools_t::rec_type_t::fnam, "INGR"}, "INGR"}, {{tools_t::rec_type_t::fnam, "LIGH"}, "LIGH"},
                        {{tools_t::rec_type_t::fnam, "LOCK"}, "LOCK"}, {{tools_t::rec_type_t::fnam, "MISC"}, "MISC"},
                        {{tools_t::rec_type_t::fnam, "NPC_"}, "NPC_"}, {{tools_t::rec_type_t::fnam, "PROB"}, "PROB"},
                        {{tools_t::rec_type_t::fnam, "RACE"}, "RACE"}, {{tools_t::rec_type_t::fnam, "REGN"}, "REGN"},
                        {{tools_t::rec_type_t::fnam, "REPA"}, "REPA"}, {{tools_t::rec_type_t::fnam, "SPEL"}, "SPEL"},
                        {{tools_t::rec_type_t::fnam, "WEAP"}, "WEAP"},
                        {{tools_t::rec_type_t::desc, "BSGN"}, "Birthsigns"},
                        {{tools_t::rec_type_t::desc, "CLAS"}, "Classes"},
                        {{tools_t::rec_type_t::desc, "RACE"}, "Races"},
                        {{tools_t::rec_type_t::indx, "SKIL"}, "Skills"},
                        {{tools_t::rec_type_t::indx, "MGEF"}, "Magic Effects"},
                    };
                    auto p2s_it = prefix_to_sub_type.find({type, prefix});
                    if (p2s_it != prefix_to_sub_type.end())
                    {
                        sub_type_counts_2[p2s_it->second]++;
                        if (done_statuses.count(rec.status))
                            sub_type_translated_2[p2s_it->second]++;
                    }
                }
            }
        }
    }

    std::map<std::string, size_t> displayed_counts;
    for (int i = 0; i < table_model_->rowCount(); ++i)
    {
        const auto * r = table_model_->row_at(i);
        if (r)
            displayed_counts[r->status]++;
    }

    status_filter_bar_->update_counts(displayed_counts, total_status_counts);

    size_t total = 0;
    size_t total_translated = 0;
    for (const auto & [t, c] : type_counts)
        total += c;
    for (const auto & [t, c] : translated_counts)
        total_translated += c;

    filter_tree_->update_counts(type_counts, translated_counts);
    filter_tree_->update_sub_type_counts(sub_type_counts_2, sub_type_translated_2);
    filter_tree_->set_total_count(total_translated, total);
}

void main_window_t::update_validation()
{
    if (current_row_ < 0)
        return;

    const auto * row_data = table_model_->row_at(current_row_);
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

    const auto aff_files = dict_dir.entryList({"*.aff"}, QDir::Files);
    for (const auto & aff_file : aff_files)
    {
        auto base_name = aff_file;
        base_name.chop(4);

        const auto dic_path = dict_dir.filePath(base_name + ".dic");
        if (!QFile::exists(dic_path))
            continue;

        spell_lang_combo_->addItem(base_name);
    }
}

void main_window_t::on_spell_lang_changed(int index)
{
    if (index <= 0)
    {
        hl_translation_->set_spell_checker(nullptr);
        return;
    }

    const auto lang_name = spell_lang_combo_->itemText(index);
    const auto app_dir = QCoreApplication::applicationDirPath();
    const auto aff_path = app_dir + "/dictionaries/" + lang_name + ".aff";
    const auto dic_path = app_dir + "/dictionaries/" + lang_name + ".dic";

    if (!spell_checker_.load(aff_path.toStdString(), dic_path.toStdString()))
        return;

    hl_translation_->set_spell_checker(&spell_checker_);
}

void main_window_t::load_config()
{
    const auto path = QCoreApplication::applicationDirPath() + "/yampt_gui.ini";
    config_.load(path.toStdString());

    move(config_.window_x, config_.window_y);
    resize(config_.window_w, config_.window_h);

    if (config_.window_maximized)
        showMaximized();

    encoding_combo_->setCurrentIndex(config_.encoding_index);

    constexpr codepage_t codepages_table[] = {
        codepage_t::windows_1250,
        codepage_t::windows_1251,
        codepage_t::windows_1252,
    };
    if (config_.encoding_index >= 0 && config_.encoding_index < 3)
        current_codepage_ = codepages_table[config_.encoding_index];

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

    if (config_.active_dict_index >= 0 && config_.active_dict_index < workspace_.slot_count())
        workspace_.set_active(config_.active_dict_index);

    rebuild_table();

    if (config_.spell_lang_index > 0 && config_.spell_lang_index < spell_lang_combo_->count())
        spell_lang_combo_->setCurrentIndex(config_.spell_lang_index);

    update_watcher_paths();
}

void main_window_t::save_config()
{
    config_.window_x = pos().x();
    config_.window_y = pos().y();
    config_.window_w = size().width();
    config_.window_h = size().height();
    config_.window_maximized = isMaximized();

    config_.encoding_index = encoding_combo_->currentIndex();
    config_.spell_lang_index = spell_lang_combo_->currentIndex();

    config_.sidebar_visible = sidebar_toggle_->isChecked();
    config_.bottom_visible = bottom_panel_toggle_->isChecked();

    config_.split_ratio = static_cast<float>(editor_panel_->get_split_ratio());

    const auto col_widths = table_view_->get_column_widths();
    for (size_t i = 0; i < col_widths.size() && i < config_.column_widths.size(); ++i)
        config_.column_widths[i] = static_cast<float>(col_widths[i]);

    config_.active_dict_index = workspace_.get_active_index();
    config_.workspace_roots = file_list_.get_roots();

    const auto path = QCoreApplication::applicationDirPath() + "/yampt_gui.ini";
    config_.save(path.toStdString());
}

void main_window_t::on_plugin_operation(const std::string & plugin_path_arg, plugin_op_t op)
{
    const auto plugin_path = plugin_path_arg;
    const auto encoding = get_current_tools_encoding();

    auto path_sep = plugin_path.find_last_of("/\\");
    auto plugin_dir = path_sep != std::string::npos ? plugin_path.substr(0, path_sep) : std::string{};
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

        dict_selection_dialog_t dialog(entries, this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        const auto selected = dialog.get_selected_paths();
        if (selected.empty())
            return;

        ensure_dicts_loaded(selected);
        dict_merger_t merger(selected);
        result = executor_.make_dict_with_base(plugin_path, merger.get_dict(), encoding);
        break;
    }
    case plugin_op_t::make_base:
    {
        auto source_sep = plugin_path.find_last_of("/\\");
        auto source_filename = source_sep != std::string::npos ? plugin_path.substr(source_sep + 1) : plugin_path;
        auto source_lower = source_filename;
        std::transform(source_lower.begin(), source_lower.end(), source_lower.begin(),
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

            plugins.push_back({entry->path, derive_display_name(*entry), entry->filename, entry->root_path, entry->workspace_subfolder});
        }

        if (plugins.empty())
            return;

        std::sort(plugins.begin(), plugins.end(),
            [](const plugin_item_t & a, const plugin_item_t & b)
            {
                return a.filename < b.filename;
            });

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
                root_label = "Workspace";

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
                    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
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
        connect(tree, &QTreeWidget::itemDoubleClicked, &dlg, [&dlg](QTreeWidgetItem * item, int) {
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

        dict_selection_dialog_t dialog(entries, this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        const auto selected = dialog.get_selected_paths();
        if (selected.empty())
            return;

        ensure_dicts_loaded(selected);
        result = executor_.convert(plugin_path, selected, encoding);
        break;
    }
    case plugin_op_t::create_plugin:
    {
        auto entries = build_dict_entries(plugin_dir);

        dict_selection_dialog_t dialog(entries, this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        const auto selected = dialog.get_selected_paths();
        if (selected.empty())
            return;

        ensure_dicts_loaded(selected);

        result = executor_.create_plugin(plugin_path, selected, encoding);
        break;
    }
    }

    auto sep = plugin_path.find_last_of("/\\");
    auto plugin_name = sep != std::string::npos ? plugin_path.substr(sep + 1) : plugin_path;

    std::string op_name;
    switch (op)
    {
    case plugin_op_t::make_dict: op_name = "Make Dict: " + plugin_name; break;
    case plugin_op_t::make_dict_with_base: op_name = "Make Dict with Base: " + plugin_name; break;
    case plugin_op_t::make_base: op_name = "Make Base: " + plugin_name; break;
    case plugin_op_t::convert: op_name = "Convert: " + plugin_name; break;
    case plugin_op_t::create_plugin: op_name = "Create: " + plugin_name; break;
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

void main_window_t::save_lua_temp()
{
    if (lua_active_path_.empty())
        return;

    if (lua_modified_indices_.empty())
        return;

    const auto tmp_path = lua_active_path_ + ".tmp";

    std::vector<l10n_entry_t> modified_entries;
    std::vector<std::string> key_order;
    for (auto idx : lua_modified_indices_)
    {
        if (idx >= lua_entries_.size())
            continue;

        modified_entries.push_back(lua_entries_[idx]);
        key_order.push_back(lua_entries_[idx].key);
    }

    yaml_l10n_writer_t writer;
    writer.write(tmp_path, modified_entries, key_order);
}

std::vector<dict_selection_dialog_t::dict_entry_t> main_window_t::build_dict_entries(const std::string & source_dir) const
{
    std::vector<dict_selection_dialog_t::dict_entry_t> entries;
    std::set<std::string> added_paths;

    auto normalize = [](std::string p) {
        std::replace(p.begin(), p.end(), '\\', '/');
        return p;
    };

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (!slot)
            continue;

        auto filename = slot->path;
        auto sep = filename.find_last_of("/\\");
        if (sep != std::string::npos)
            filename = filename.substr(sep + 1);

        auto kind = workspace_.is_base_slot(i) ? dict_kind_t::base : dict_kind_t::user;

        entries.push_back({filename, slot->path, kind, false});
        added_paths.insert(normalize(slot->path));
    }

    for (const auto * ws_entry : file_list_.workspace_files())
    {
        if (added_paths.count(normalize(ws_entry->path)))
            continue;

        if (ws_entry->type != file_type_t::base_dict && ws_entry->type != file_type_t::user_dict)
            continue;

        auto kind = (ws_entry->type == file_type_t::base_dict) ? dict_kind_t::base : dict_kind_t::user;
        entries.push_back({ws_entry->filename, ws_entry->path, kind, false});
    }

    if (!source_dir.empty())
    {
        const auto target = normalize(source_dir);

        for (auto & entry : entries)
        {
            auto dir = normalize(entry.path);
            auto dir_sep = dir.find_last_of('/');
            if (dir_sep != std::string::npos)
                dir = dir.substr(0, dir_sep);

            if (dir == target)
                entry.checked = true;
        }
    }

    return entries;
}

void main_window_t::ensure_dicts_loaded(const std::vector<std::string> & paths)
{
    for (const auto & path : paths)
    {
        bool already_loaded = false;
        for (int i = 0; i < workspace_.slot_count(); ++i)
        {
            const auto * slot = workspace_.get_slot(i);
            if (slot && slot->path == path)
            {
                already_loaded = true;
                break;
            }
        }

        if (already_loaded)
            continue;

        auto kind = (path.find("_BASE_") != std::string::npos) ? dict_kind_t::base : dict_kind_t::user;
        workspace_.load_dict(path, kind);

        auto * new_slot = workspace_.get_slot(workspace_.slot_count() - 1);
        if (!new_slot)
            continue;

        for (auto & [type, chapter] : new_slot->data)
        {
            for (auto & entry : chapter.records)
            {
                entry.key_text = decode_to_utf8(entry.key_text, current_codepage_);
                entry.old_text = decode_to_utf8(entry.old_text, current_codepage_);
                entry.new_text = decode_to_utf8(entry.new_text, current_codepage_);
                    if (!entry.adapted_from.empty())
                        entry.adapted_from = decode_to_utf8(entry.adapted_from, current_codepage_);
            }
        }
    }
}

tools_t::encoding_t main_window_t::get_current_tools_encoding() const
{
    constexpr tools_t::encoding_t encodings[] = {
        tools_t::encoding_t::windows_1250,
        tools_t::encoding_t::windows_1251,
        tools_t::encoding_t::windows_1252,
    };

    const int index = encoding_combo_->currentIndex();
    if (index < 0 || index >= static_cast<int>(std::size(encodings)))
        return tools_t::encoding_t::windows_1252;

    return encodings[index];
}

void main_window_t::on_item_clicked(const std::string & path)
{
    const auto * entry = file_list_.get(path);
    if (!entry)
        return;

    if (entry->type == file_type_t::plugin)
    {
        table_model_->rebuild({});
        current_row_ = -1;
        filter_tree_->set_display_mode(filter_tree_t::display_mode_t::empty);
        filter_tree_->setEnabled(false);
        active_file_label_->clear();
        status_filter_bar_->set_dict_mode(status_filter_bar_t::dict_mode_t::none);
        search_label_->setEnabled(false);
        search_field_->setEnabled(false);
        case_sensitive_check_->setEnabled(false);
        regex_check_->setEnabled(false);
        search_col_key_->setEnabled(false);
        search_col_original_->setEnabled(false);
        search_col_translation_->setEnabled(false);
        return;
    }

    if (entry->type == file_type_t::lua_l10n)
    {
        auto norm_path = path;
        std::replace(norm_path.begin(), norm_path.end(), '\\', '/');

        auto active_norm = lua_active_path_;
        std::replace(active_norm.begin(), active_norm.end(), '\\', '/');
        if (norm_path == active_norm)
            return;

        commit_current_edit();
        save_lua_temp();

        yaml_l10n_reader_t reader;
        if (!reader.load(path))
        {
            table_model_->rebuild({});
            active_file_label_->setText(QString::fromStdString(path));
            return;
        }

        lua_active_path_ = path;
        lua_entries_.clear();
        lua_modified_indices_.clear();
        workspace_.set_active(-1);

        const auto & source_entries = reader.source_entries();
        for (const auto & src : source_entries)
        {
            l10n_entry_t e;
            e.key = src.key;
            e.value = src.value;
            lua_entries_.push_back(std::move(e));
        }

        const auto tmp_path = path + ".tmp";
        yaml_l10n_reader_t tmp_reader;
        if (tmp_reader.load(tmp_path))
        {
            std::unordered_map<std::string, std::string> tmp_map;
            for (const auto & e : tmp_reader.source_entries())
                tmp_map[e.key] = e.value;

            for (size_t i = 0; i < lua_entries_.size(); ++i)
            {
                auto it = tmp_map.find(lua_entries_[i].key);
                if (it != tmp_map.end())
                {
                    lua_entries_[i].value = it->second;
                    lua_modified_indices_.insert(i);
                }
            }
        }

        std::vector<table_row_t> rows;
        for (size_t i = 0; i < lua_entries_.size(); ++i)
        {
            table_row_t row;
            row.type = tools_t::rec_type_t::lua;
            row.key_text = lua_entries_[i].key;
            row.old_text = source_entries[i].value;
            row.new_text = lua_modified_indices_.count(i) ? lua_entries_[i].value : "";

            if (lua_modified_indices_.count(i))
                row.status = "in_progress";
            else
                row.status = "untranslated";

            row.chapter_index = i;
            rows.push_back(std::move(row));
        }

        table_model_->rebuild(std::move(rows));
        active_file_label_->setText(QString::fromStdString(lua_active_path_));
        filter_tree_->set_display_mode(filter_tree_t::display_mode_t::all_only);
        filter_tree_->setEnabled(true);
        status_filter_bar_->set_dict_mode(status_filter_bar_t::dict_mode_t::user);
        search_label_->setEnabled(true);
        search_field_->setEnabled(true);
        case_sensitive_check_->setEnabled(true);
        regex_check_->setEnabled(true);
        search_col_key_->setEnabled(true);
        search_col_original_->setEnabled(true);
        search_col_translation_->setEnabled(true);
        current_row_ = -1;
        return;
    }

    lua_active_path_.clear();
    lua_entries_.clear();
    lua_modified_indices_.clear();

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot && slot->path == path)
        {
            commit_current_edit();
            workspace_.set_active(i);
            rebuild_table();
            editor_panel_->original_view()->clear();
            editor_panel_->translation_editor()->clear();
            editor_panel_->clear_adapted_from();
            validation_indicator_->clear();
            annotations_panel_->clear();
            history_panel_->clear();
            book_preview_->clear();
            current_row_ = -1;
            return;
        }
    }

    int old_count = workspace_.slot_count();

    auto kind = dict_kind_t::user;
    if (path.find("_BASE_") != std::string::npos)
        kind = dict_kind_t::base;

    workspace_.load_dict(path, kind);

    if (workspace_.slot_count() <= old_count)
        return;

    file_list_.set_loaded(path, true);

    auto * new_slot = workspace_.get_slot(workspace_.slot_count() - 1);
    if (new_slot)
    {
        for (auto & [type, chapter] : new_slot->data)
        {
            for (auto & rec : chapter.records)
            {
                rec.key_text = decode_to_utf8(rec.key_text, current_codepage_);
                rec.old_text = decode_to_utf8(rec.old_text, current_codepage_);
                rec.new_text = decode_to_utf8(rec.new_text, current_codepage_);
                if (!rec.adapted_from.empty())
                    rec.adapted_from = decode_to_utf8(rec.adapted_from, current_codepage_);
            }
        }
    }

    std::vector<dict_source_t> sources;
    for (const auto & dict_slot : workspace_.get_all_slots())
        sources.push_back({&dict_slot.data, dict_slot.path});
    annotation_manager_.rebuild(sources);

    commit_current_edit();
    workspace_.set_active(workspace_.slot_count() - 1);
    rebuild_table();
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

    auto norm_lua = lua_active_path_;
    std::replace(norm_lua.begin(), norm_lua.end(), '\\', '/');

    if (!lua_active_path_.empty() && norm_path == norm_lua)
    {
        commit_current_edit();
        save_lua_temp();

        auto * fe = file_list_.get(lua_active_path_);
        if (fe)
        {
            fe->dirty = false;
            sidebar_->update_item_text(fe->path, derive_display_name(*fe));
        }

        set_dirty(false);
        return;
    }

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot && slot->path == path)
        {
            save_dict_encoded(i);

            auto * fe = file_list_.get(slot->path);
            if (fe)
                sidebar_->update_item_text(fe->path, derive_display_name(*fe));

            log_tab_->append_log("save", "saved \"" + slot->path + "\"\r\n");
            if (!workspace_.has_any_unsaved())
                set_dirty(false);
            return;
        }
    }
}

void main_window_t::on_unload_requested(const std::string & path)
{
    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot && slot->path == path)
        {
            on_unload_slot(i);
            return;
        }
    }

    file_list_.remove(path);
    rebuild_sidebar();
}

void main_window_t::on_delete_requested(const std::string & path)
{
    auto sep = path.find_last_of("/\\");
    auto filename = sep != std::string::npos ? path.substr(sep + 1) : path;

    auto answer = QMessageBox::question(
        this, "Delete File",
        QString("Delete \"%1\" from disk?").arg(QString::fromStdString(filename)),
        QMessageBox::Yes | QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    QFile::remove(QString::fromStdString(path));

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot && slot->path == path)
        {
            workspace_.unload_dict(i);
            break;
        }
    }

    file_list_.remove(path);
    rebuild_sidebar();
    rebuild_table();
    scan_workspace();
}

void main_window_t::closeEvent(QCloseEvent * event)
{
    if (workspace_.has_any_unsaved())
    {
        auto answer = QMessageBox::question(
            this, "Unsaved Changes",
            "You have unsaved dictionary changes. What would you like to do?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (answer == QMessageBox::Cancel)
        {
            event->ignore();
            return;
        }

        if (answer == QMessageBox::Save)
        {
            for (int i = 0; i < workspace_.slot_count(); ++i)
            {
                const auto * slot = workspace_.get_slot(i);
                if (slot && slot->dirty)
                    save_dict_encoded(i);
            }
        }
    }

    commit_current_edit();
    save_lua_temp();
    save_config();
    QMainWindow::closeEvent(event);
}
