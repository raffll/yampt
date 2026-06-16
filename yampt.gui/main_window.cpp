#include "main_window.hpp"
#include "annotation_highlighter.hpp"
#include "annotations_panel.hpp"
#include "book_preview.hpp"
#include "composite_highlighter.hpp"
#include "dict_selection_dialog.hpp"
#include "editor_panel.hpp"
#include "filter_bar.hpp"
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
#include "yaml_l10n_reader.hpp"

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
#include <QInputDialog>
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
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>
#include <filesystem>
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

    open_action_ = new QAction("Open &User Dictionary...", this);
    open_action_->setShortcut(QKeySequence("Ctrl+O"));
    file_menu->addAction(open_action_);

    open_base_action_ = new QAction("Open &Base Dictionary...", this);
    file_menu->addAction(open_base_action_);

    load_plugin_action_ = new QAction("Load &Plugin...", this);
    file_menu->addAction(load_plugin_action_);

    load_archive_action_ = new QAction("Load Mod from &Archive...", this);
    file_menu->addAction(load_archive_action_);

    load_l10n_action_ = new QAction("Load l10n &Folder...", this);
    file_menu->addAction(load_l10n_action_);

    save_action_ = new QAction("&Save", this);
    save_action_->setShortcut(QKeySequence("Ctrl+S"));
    file_menu->addAction(save_action_);

    save_all_action_ = new QAction("Save &All", this);
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

    filter_bar_ = new filter_bar_t(central_widget);
    status_filter_bar_ = new status_filter_bar_t(central_widget);
    central_layout->addWidget(filter_bar_);
    central_layout->addWidget(status_filter_bar_);

    central_splitter_ = new QSplitter(Qt::Horizontal, central_widget);
    central_layout->addWidget(central_splitter_, 1);

    setCentralWidget(central_widget);

    left_splitter_ = new QSplitter(Qt::Vertical, central_splitter_);
    sidebar_ = new sidebar_widget_t(left_splitter_);
    left_splitter_->addWidget(sidebar_);

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

    connect(open_action_, &QAction::triggered, this, &main_window_t::on_open_user_dict);
    connect(open_base_action_, &QAction::triggered, this, &main_window_t::on_open_base_dict);
    connect(load_plugin_action_, &QAction::triggered, this, &main_window_t::on_load_plugin);
    connect(load_archive_action_, &QAction::triggered, this, &main_window_t::on_load_archive);
    connect(load_l10n_action_, &QAction::triggered, this, [this]() {
        auto folder = QFileDialog::getExistingDirectory(this, "Load l10n Folder");
        if (folder.isEmpty())
            return;

        load_l10n_folder(folder.toStdString());
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
    connect(sidebar_, &sidebar_widget_t::unload_requested, this, &main_window_t::on_unload_requested);
    connect(sidebar_, &sidebar_widget_t::delete_requested, this, &main_window_t::on_delete_requested);

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

        int next_row = current_row_ + 1;
        int row_count = table_model_->rowCount();
        if (next_row >= row_count)
            next_row = row_count - 1;

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

        int next_row = current_row_ + 1;
        int row_count = table_model_->rowCount();
        if (next_row >= row_count)
            next_row = row_count - 1;

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

    connect(filter_bar_, &filter_bar_t::filters_changed, this, &main_window_t::on_filters_changed);
    connect(filter_bar_, &filter_bar_t::all_reset_requested, this, [this]() {
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
        rebuild_table();
        load_record(current_row_);
    });

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

    int active = workspace_.get_active_index();
    if (active < 0)
        return;

    if (!workspace_.is_user_slot(active))
        return;

    const auto * slot = workspace_.get_slot(active);
    save_dict_encoded(active);
    rebuild_sidebar();

    if (slot)
        log_tab_->append_log("save", "saved \"" + slot->path + "\"\r\n");

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
            log_msg += "saved \"" + slot->path + "\"\r\n";
        }
    }

    rebuild_sidebar();

    if (!log_msg.empty())
        log_tab_->append_log("save all", log_msg);

    if (!workspace_.has_any_unsaved())
        set_dirty(false);
}

void main_window_t::on_open()
{
    on_open_user_dict();
}

void main_window_t::on_open_user_dict()
{
    const auto path = QFileDialog::getOpenFileName(
        this, "Open User Dict", "", "Dictionary Files (*.json *.xml);;All Files (*.*)");

    if (path.isEmpty())
        return;

    int old_count = workspace_.slot_count();
    workspace_.load_dict(path.toStdString(), dict_kind_t::user);

    if (workspace_.slot_count() > old_count)
    {
        auto * new_slot = workspace_.get_slot(workspace_.slot_count() - 1);
        if (new_slot)
        {
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

        std::vector<dict_source_t> sources;
        for (const auto & dict_slot : workspace_.get_all_slots())
            sources.push_back({&dict_slot.data, dict_slot.path});
        annotation_manager_.rebuild(sources);
    }

    rebuild_sidebar();
    rebuild_table();
}

void main_window_t::on_open_base_dict()
{
    const auto path = QFileDialog::getOpenFileName(
        this, "Open Base Dict", "", "Dictionary Files (*.json *.xml);;All Files (*.*)");

    if (path.isEmpty())
        return;

    int old_count = workspace_.slot_count();
    workspace_.load_dict(path.toStdString(), dict_kind_t::base);

    if (workspace_.slot_count() > old_count)
    {
        auto * new_slot = workspace_.get_slot(workspace_.slot_count() - 1);
        if (new_slot)
        {
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

        std::vector<dict_source_t> sources;
        for (const auto & dict_slot : workspace_.get_all_slots())
            sources.push_back({&dict_slot.data, dict_slot.path});
        annotation_manager_.rebuild(sources);
    }

    rebuild_sidebar();
    rebuild_table();
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
    type_filter_ = filter_bar_->get_active_types();
    type_filter_solo_ = filter_bar_->is_solo();
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

    const auto * slot = workspace_.get_active_slot();
    if (!slot)
    {
        table_model_->rebuild({});
        progress_label_->clear();
        filter_bar_->setEnabled(false);
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

    filter_bar_->setEnabled(true);
    search_label_->setEnabled(true);
    search_field_->setEnabled(true);
    case_sensitive_check_->setEnabled(true);
    regex_check_->setEnabled(true);
    search_col_key_->setEnabled(true);
    search_col_original_->setEnabled(true);
    search_col_translation_->setEnabled(true);

    const auto active_sub_types = filter_bar_->get_active_sub_types();
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

    std::map<std::string, size_t> displayed_status_counts;
    std::map<std::string, size_t> total_status_counts;
    for (const auto & [type, chapter] : slot->data)
    {
        for (const auto & rec : chapter.records)
            total_status_counts[rec.status]++;
    }

    for (int i = 0; i < table_model_->rowCount(); ++i)
    {
        const auto * r = table_model_->row_at(i);
        if (r)
            displayed_status_counts[r->status]++;
    }

    size_t total = 0;
    size_t total_translated = 0;
    for (const auto & [t, c] : type_counts)
        total += c;
    for (const auto & [t, c] : translated_counts)
        total_translated += c;

    filter_bar_->update_counts(type_counts, translated_counts);
    filter_bar_->set_total_count(total_translated, total);
    status_filter_bar_->update_counts(displayed_status_counts, total_status_counts);

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
        progress_label_->setText(QString("%1 / %2 (%3%) \u2022 %4 shown")
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

    if (workspace_.get_active_index() >= 0)
    {
        auto * slot = workspace_.get_slot(workspace_.get_active_index());
        if (slot && !slot->dirty)
        {
            slot->dirty = true;
            file_list_.set_dirty(slot->path, true);
            rebuild_sidebar();
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

    it->second.records[row_data->chapter_index].new_text = new_text_str;
    it->second.records[row_data->chapter_index].status = "in_progress";
    slot->dirty = true;
    slot->modified_records.insert({row_data->type, row_data->chapter_index});
    set_dirty(true);

    if (new_text_str != it->second.records[row_data->chapter_index].old_text)
    {
        int propagated = propagate_translation(it->second.records[row_data->chapter_index].old_text, new_text_str);
        if (propagated > 0)
        {
            it->second.records[row_data->chapter_index].status = "propagated";
            slot->dirty = true;
            statusBar()->showMessage(QString("Propagated to %1 entries").arg(propagated), 5000);

            const auto saved_key = row_data->key_text;
            const auto saved_type = row_data->type;
            rebuild_table();

            for (int i = 0; i < table_model_->rowCount(); ++i)
            {
                const auto * r = table_model_->row_at(i);
                if (r && r->key_text == saved_key && r->type == saved_type)
                {
                    table_view_->selectRow(i);
                    break;
                }
            }

            rebuild_sidebar();
            update_status_counts();
            loaded_text_ = current_text;
            return;
        }
    }

    table_model_->update_row(current_row_, new_text_str, "in_progress");
    loaded_text_ = current_text;
    rebuild_sidebar();
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

    const bool is_base = workspace_.is_base_slot(workspace_.get_active_index());
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
	const auto active_path = workspace_.get_active_index() >= 0
		? workspace_.get_slot(workspace_.get_active_index())->path
		: std::string{};
	const auto model = build_render_model(file_list_, active_path);
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

    for (const auto & [type, chapter] : slot->data)
    {
        for (const auto & rec : chapter.records)
        {
            total_status_counts[rec.status]++;
            type_counts[type]++;

            if (done_statuses.count(rec.status))
                translated_counts[type]++;
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

    filter_bar_->update_counts(type_counts, translated_counts);
    filter_bar_->set_total_count(total_translated, total);
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

    for (const auto & p : config_.user_dict_paths)
        workspace_.load_dict(p, dict_kind_t::user);

    for (const auto & p : config_.base_dict_paths)
        workspace_.load_dict(p, dict_kind_t::base);

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        auto * s = workspace_.get_slot(i);
        if (!s)
            continue;

        for (auto & [type, chapter] : s->data)
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

    if (config_.active_dict_index >= 0 && config_.active_dict_index < workspace_.slot_count())
        workspace_.set_active(config_.active_dict_index);

    if (workspace_.slot_count() > 0)
    {
        std::vector<dict_source_t> sources;
        for (const auto & dict_slot : workspace_.get_all_slots())
            sources.push_back({&dict_slot.data, dict_slot.path});
        annotation_manager_.rebuild(sources);
    }

    rebuild_sidebar();
    rebuild_table();

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (!slot)
            continue;

        file_list_.add(slot->path);
        file_list_.set_loaded(slot->path, true);
    }

    for (const auto & p : config_.plugin_paths)
    {
        if (!QFile::exists(QString::fromStdString(p)))
            continue;

        auto & entry = file_list_.add(p);
        entry.loaded = true;

        std::error_code ec;
        auto file_size = std::filesystem::file_size(p, ec);
        if (!ec)
            entry.language_tag = file_list_t::detect_language(entry.filename, file_size);

        log_tab_->append_log("restore",
            "filename=\"" + entry.filename +
            "\" size=" + std::to_string(ec ? 0 : file_size) +
            " lang=" + (entry.language_tag.empty() ? "none" : entry.language_tag) +
            " ec=" + (ec ? ec.message() : "ok") + "\r\n");
    }

    rebuild_sidebar();
    scan_workspace();

    if (config_.spell_lang_index > 0 && config_.spell_lang_index < spell_lang_combo_->count())
        spell_lang_combo_->setCurrentIndex(config_.spell_lang_index);
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

    config_.user_dict_paths.clear();
    config_.base_dict_paths.clear();

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (!slot)
            continue;

        const auto * fe = file_list_.get(slot->path);
        if (fe && fe->is_workspace)
            continue;

        if (!fe)
        {
            auto normalized = slot->path;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";
            std::replace(workspace_dir.begin(), workspace_dir.end(), '\\', '/');
            if (normalized.find(workspace_dir) == 0)
                continue;
        }

        if (workspace_.is_user_slot(i))
            config_.user_dict_paths.push_back(slot->path);
        else
            config_.base_dict_paths.push_back(slot->path);
    }

    config_.plugin_paths.clear();
    for (const auto * entry : file_list_.loaded_non_workspace())
    {
        if (entry->type == file_type_t::plugin)
            config_.plugin_paths.push_back(entry->path);
    }

    const auto path = QCoreApplication::applicationDirPath() + "/yampt_gui.ini";
    config_.save(path.toStdString());
}

void main_window_t::on_load_plugin()
{
    const auto paths = QFileDialog::getOpenFileNames(
        this, "Load Plugin", "", "Plugin Files (*.esm *.esp *.omwgame *.omwaddon);;All Files (*.*)");

    if (paths.isEmpty())
        return;

    for (const auto & path : paths)
    {
        auto & entry = file_list_.add(path.toStdString());
        entry.loaded = true;

        std::error_code ec;
        auto file_size = std::filesystem::file_size(entry.path, ec);
        if (!ec)
            entry.language_tag = file_list_t::detect_language(entry.filename, file_size);

        log_tab_->append_log("load plugin",
            "filename=\"" + entry.filename +
            "\" size=" + std::to_string(ec ? 0 : file_size) +
            " lang=" + (entry.language_tag.empty() ? "none" : entry.language_tag) +
            " ec=" + (ec ? ec.message() : "ok") + "\r\n");
    }

    rebuild_sidebar();
}

void main_window_t::on_load_archive()
{
    auto archive_path = QFileDialog::getOpenFileName(
        this, "Load Mod from Archive", "",
        "Archive files (*.zip *.7z *.rar);;All files (*.*)");

    if (archive_path.isEmpty())
        return;

    const QString sevenzip = "C:/Program Files/7-Zip/7z.exe";
    if (!QFile::exists(sevenzip))
    {
        QMessageBox::critical(this, "Error", "7-Zip not found at C:\\Program Files\\7-Zip\\7z.exe");
        return;
    }

    auto temp_dir = QDir::tempPath() + "/yampt_extract_" + QUuid::createUuid().toString(QUuid::Id128);
    QDir().mkpath(temp_dir);

    QProcess proc;
    proc.start(sevenzip, {"x", archive_path, "-o" + temp_dir, "-y"});
    proc.waitForFinished(60000);
    if (proc.exitCode() != 0)
    {
        QMessageBox::critical(this, "Extraction Error", proc.readAllStandardError());
        QDir(temp_dir).removeRecursively();
        return;
    }

    QDir root(temp_dir);
    auto entries = root.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
    QString effective_root = temp_dir;
    if (entries.size() == 1)
    {
        auto single_path = temp_dir + "/" + entries.first();
        if (QFileInfo(single_path).isDir())
            effective_root = single_path;
    }

    auto archive_name = QFileInfo(archive_path).completeBaseName();
    archive_name.remove(QRegularExpression("-\\d{4,}[\\d\\-]*$"));

    auto workspace_dir = QCoreApplication::applicationDirPath() + "/workspace/" + archive_name;

    if (QDir(workspace_dir).exists())
    {
        auto answer = QMessageBox::question(this, "Folder Exists",
            QString("\"%1\" already exists. Overwrite?").arg(archive_name),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            QDir(temp_dir).removeRecursively();
            return;
        }
    }

    static const QStringList supported = {"esm", "esp", "omwaddon", "omwgame", "omwscripts", "lua", "yaml"};

    QDirIterator it(effective_root, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        auto suffix = it.fileInfo().suffix().toLower();
        if (!supported.contains(suffix))
            continue;

        auto relative = QDir(effective_root).relativeFilePath(it.filePath());
        auto target = workspace_dir + "/" + relative;

        if (QFile::exists(target))
        {
            auto dir = QFileInfo(target).path();
            auto base = QFileInfo(target).completeBaseName();
            auto ext = QFileInfo(target).suffix();
            int n = 1;
            while (QFile::exists(target))
            {
                target = QString("%1/%2_%3.%4").arg(dir).arg(base).arg(n).arg(ext);
                ++n;
            }
        }

        QDir().mkpath(QFileInfo(target).path());
        QFile::copy(it.filePath(), target);
    }

    QDir(temp_dir).removeRecursively();

    scan_workspace();
}

void main_window_t::on_plugin_operation(const std::string & plugin_path_arg, plugin_op_t op)
{
    const auto plugin_path = plugin_path_arg;
    const auto encoding = get_current_tools_encoding();

    auto path_sep = plugin_path.find_last_of("/\\");
    auto plugin_dir = path_sep != std::string::npos ? plugin_path.substr(0, path_sep) : std::string{};
    std::replace(plugin_dir.begin(), plugin_dir.end(), '\\', '/');

    auto workspace_base = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";
    std::replace(workspace_base.begin(), workspace_base.end(), '\\', '/');

    std::string workspace_folder;
    if (plugin_dir.find(workspace_base) == 0)
        workspace_folder = plugin_dir.substr(workspace_base.size());

    std::string output_dir;
    if (!workspace_folder.empty())
    {
        output_dir = workspace_base + workspace_folder + "/";
    }
    executor_.set_output_dir(output_dir);

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
        auto entries = build_dict_entries(workspace_folder);

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

        QStringList plugin_names;
        std::vector<std::string> plugin_paths;
        int best_match_row = 0;

        for (const auto * entry : file_list_.all())
        {
            if (entry->type != file_type_t::plugin)
                continue;

            if (entry->path == plugin_path)
                continue;

            auto name_lower = entry->filename;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (name_lower == source_lower)
                best_match_row = static_cast<int>(plugin_names.size());

            auto display = entry->filename;
            if (!entry->language_tag.empty())
                display += " [" + entry->language_tag + "]";

            plugin_names.append(QString::fromStdString(display));
            plugin_paths.push_back(entry->path);
        }

        if (plugin_names.isEmpty())
            return;

        QDialog dlg(this);
        dlg.setWindowTitle("Make Base");
        dlg.setModal(true);

        auto * dlg_layout = new QVBoxLayout(&dlg);
        dlg_layout->addWidget(new QLabel("Select the native ESM:", &dlg));

        auto * list = new QListWidget(&dlg);
        list->addItems(plugin_names);
        list->setCurrentRow(best_match_row);
        dlg_layout->addWidget(list);

        auto * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        dlg_layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        connect(list, &QListWidget::itemDoubleClicked, &dlg, &QDialog::accept);

        if (dlg.exec() != QDialog::Accepted)
            return;

        int sel_idx = list->currentRow();
        if (sel_idx < 0)
            return;

        const auto & native_path = plugin_paths[sel_idx];
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
        auto entries = build_dict_entries(workspace_folder);

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
        auto entries = build_dict_entries(workspace_folder);

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
    const auto app_dir = QCoreApplication::applicationDirPath();
    QDir workspace_dir(app_dir + "/workspace");

    if (!workspace_dir.exists())
    {
        file_list_.scan_workspace(workspace_dir.absolutePath().toStdString());
        rebuild_sidebar();
        return;
    }

    std::set<std::string> loaded_paths;
    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot)
            loaded_paths.insert(slot->path);
    }

    auto load_workspace_dicts = [&](const QDir & dir)
    {
        const auto files = dir.entryList({"*.json", "*.xml"}, QDir::Files);
        for (const auto & f : files)
        {
            auto path = dir.absoluteFilePath(f).toStdString();

            if (loaded_paths.count(path))
                continue;

            auto kind = (path.find("_BASE_") != std::string::npos) ? dict_kind_t::base : dict_kind_t::user;
            workspace_.load_dict(path, kind);

            auto * new_slot = workspace_.get_slot(workspace_.slot_count() - 1);
            if (new_slot)
            {
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
    };

    load_workspace_dicts(workspace_dir);

    const auto subdirs = workspace_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto & sub : subdirs)
        load_workspace_dicts(QDir(workspace_dir.absoluteFilePath(sub)));

    file_list_.scan_workspace(workspace_dir.absolutePath().toStdString());
    rebuild_sidebar();

    std::vector<dict_source_t> sources;
    for (const auto & dict_slot : workspace_.get_all_slots())
        sources.push_back({&dict_slot.data, dict_slot.path});
    annotation_manager_.rebuild(sources);
}

void main_window_t::load_l10n_folder(const std::string & folder_path)
{
    namespace fs = std::filesystem;

    fs::path dir(folder_path);
    fs::path source_path = dir / "en.yaml";
    if (!fs::exists(source_path))
    {
        QMessageBox::critical(this, "Error",
            QString("en.yaml not found in \"%1\"").arg(QString::fromStdString(folder_path)));
        return;
    }

    std::vector<std::string> target_names;
    for (const auto & entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file())
            continue;

        auto ext = entry.path().extension().string();
        if (ext != ".yaml" && ext != ".yml")
            continue;

        auto stem = entry.path().stem().string();
        if (stem == "en")
            continue;

        target_names.push_back(stem);
    }

    std::string target_lang;

    if (target_names.size() > 1)
    {
        QStringList items;
        for (const auto & name : target_names)
            items.append(QString::fromStdString(name));

        bool ok = false;
        auto selected = QInputDialog::getItem(this, "Select Target Language",
            "Multiple target languages found. Select one:", items, 0, false, &ok);

        if (!ok || selected.isEmpty())
            return;

        target_lang = selected.toStdString();
    }
    else if (target_names.size() == 1)
    {
        target_lang = target_names[0];
    }
    else
    {
        bool ok = false;
        auto code = QInputDialog::getText(this, "Target Language",
            "No target YAML found. Enter language code (e.g. pl, de, fr):",
            QLineEdit::Normal, "", &ok);

        if (!ok || code.isEmpty())
            return;

        target_lang = code.toStdString();
    }

    fs::path target_path = dir / (target_lang + ".yaml");

    yaml_l10n_reader_t reader;
    std::string target_str = fs::exists(target_path) ? target_path.string() : "";
    if (!reader.load(source_path.string(), target_str))
    {
        QMessageBox::critical(this, "Error", "Failed to parse en.yaml");
        return;
    }

    const auto & source_entries = reader.source_entries();
    const auto & target_entries = reader.target_entries();

    std::unordered_map<std::string, std::string> target_map;
    for (const auto & te : target_entries)
        target_map[te.key] = te.value;

    auto lua_type = static_cast<tools_t::rec_type_t>(gui_rec_type_lua);

    dict_slot_t slot;
    slot.path = target_path.string();
    auto & chapter = slot.data[lua_type];

    for (const auto & se : source_entries)
    {
        tools_t::record_entry_t entry;
        entry.key_text = se.key;
        entry.old_text = se.value;

        auto it = target_map.find(se.key);
        if (it != target_map.end() && !it->second.empty())
        {
            entry.new_text = it->second;
            entry.status = "translated";
        }
        else
        {
            entry.new_text = se.value;
            entry.status = "untranslated";
        }

        chapter.insert(entry);
    }

    workspace_.add_slot(std::move(slot), dict_kind_t::user);

    filter_bar_->set_lua_button_visible(true);
    rebuild_sidebar();
    rebuild_table();
}

std::vector<dict_selection_dialog_t::dict_entry_t> main_window_t::build_dict_entries(const std::string & workspace_folder) const
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

    if (!workspace_folder.empty())
    {
        const auto app_dir = QCoreApplication::applicationDirPath().toStdString();
        const auto target_dir = app_dir + "/workspace/" + workspace_folder;

        for (auto & entry : entries)
        {
            auto dir = entry.path;
            auto dir_sep = dir.find_last_of("/\\");
            if (dir_sep != std::string::npos)
                dir = dir.substr(0, dir_sep);

            if (dir == target_dir)
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
        filter_bar_->setEnabled(false);
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
    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot && slot->path == path)
        {
            save_dict_encoded(i);
            rebuild_sidebar();
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
    save_config();
    QMainWindow::closeEvent(event);
}
