#include "main_window.hpp"
#include "annotation_highlighter.hpp"
#include "annotations_panel.hpp"
#include "book_preview.hpp"
#include "dict_selection_dialog.hpp"
#include "editor_panel.hpp"
#include "filter_bar.hpp"
#include "history_panel.hpp"
#include "hyperlink_highlighter.hpp"
#include "log_tab.hpp"
#include "mwscript_highlighter.hpp"
#include "record_table_view.hpp"
#include "sidebar_widget.hpp"
#include "spell_check_highlighter.hpp"
#include "spell_context_menu.hpp"
#include "status_filter_bar.hpp"
#include "validation_indicator.hpp"

#include "../yampt/dict_merger.hpp"
#include "../yampt/dict_writer.hpp"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextOption>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <filesystem>
#include <set>
#include <set>

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

    open_action_ = new QAction("&Open", this);
    open_action_->setShortcut(QKeySequence("Ctrl+O"));
    file_menu->addAction(open_action_);

    open_base_action_ = new QAction("Open &Base...", this);
    file_menu->addAction(open_base_action_);

    load_plugin_action_ = new QAction("Load &Plugin...", this);
    file_menu->addAction(load_plugin_action_);

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

    whitespace_toggle_ = new QAction("Show &Whitespace", this);
    whitespace_toggle_->setCheckable(true);
    whitespace_toggle_->setChecked(false);
    view_menu->addAction(whitespace_toggle_);

    auto * toolbar = new QToolBar(this);
    toolbar->setMovable(false);

    toolbar->addWidget(new QLabel("Search:", this));
    search_field_ = new QLineEdit(this);
    search_field_->setPlaceholderText("Search...");
    toolbar->addWidget(search_field_);

    case_sensitive_check_ = new QCheckBox("Aa", this);
    toolbar->addWidget(case_sensitive_check_);

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

    validation_indicator_ = new validation_indicator_t(this);
    statusBar()->addPermanentWidget(validation_indicator_);

    central_splitter_->addWidget(left_splitter_);
    central_splitter_->addWidget(right_splitter_);
    central_splitter_->setSizes({250, 1030});

    syntax_hl_original_ = new mwscript_highlighter_t(editor_panel_->original_view()->document());
    spell_hl_ = new spell_check_highlighter_t(editor_panel_->translation_editor()->document());

    spell_menu_ = new spell_context_menu_t(&spell_checker_, spell_hl_);

    connect(sidebar_toggle_, &QAction::toggled, left_splitter_, &QWidget::setVisible);
    connect(bottom_panel_toggle_, &QAction::toggled, editor_panel_, &QWidget::setVisible);
    connect(whitespace_toggle_, &QAction::toggled, this, &main_window_t::on_whitespace_toggled);

    connect(open_action_, &QAction::triggered, this, &main_window_t::on_open_user_dict);
    connect(open_base_action_, &QAction::triggered, this, &main_window_t::on_open_base_dict);
    connect(load_plugin_action_, &QAction::triggered, this, &main_window_t::on_load_plugin);
    connect(save_action_, &QAction::triggered, this, &main_window_t::on_save);
    connect(save_all_action_, &QAction::triggered, this, &main_window_t::on_save_all);
    connect(quit_action, &QAction::triggered, this, &QWidget::close);
    connect(find_action_, &QAction::triggered, this, &main_window_t::on_find);
    connect(next_search_action_, &QAction::triggered, this, &main_window_t::on_next_search);
    connect(prev_search_action_, &QAction::triggered, this, &main_window_t::on_prev_search);
    connect(refresh_action_, &QAction::triggered, this, &main_window_t::on_refresh);
    connect(escape_action_, &QAction::triggered, this, &main_window_t::on_escape);

    connect(search_field_, &QLineEdit::textChanged, this, &main_window_t::on_search_changed);
    connect(case_sensitive_check_, &QCheckBox::checkStateChanged, this, [this]() { on_case_sensitive_changed(0); });
    connect(encoding_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &main_window_t::on_encoding_changed);
    connect(spell_lang_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &main_window_t::on_spell_lang_changed);

    connect(sidebar_, &sidebar_widget_t::slot_clicked, this, &main_window_t::on_slot_clicked);
    connect(sidebar_, &sidebar_widget_t::save_requested, this, [this](int index) {
        save_dict_encoded(index);
        if (!workspace_.has_any_unsaved())
            set_dirty(false);
        rebuild_sidebar();
    });
    connect(sidebar_, &sidebar_widget_t::unload_requested, this, &main_window_t::on_unload_slot);
    connect(sidebar_, &sidebar_widget_t::plugin_operation_requested, this, &main_window_t::on_plugin_operation);
    connect(sidebar_, &sidebar_widget_t::plugin_unload_requested, this, &main_window_t::on_plugin_unload);
    connect(sidebar_, &sidebar_widget_t::plugin_selected, this, [this]() {
        table_model_->rebuild({});
        current_row_ = -1;
    });
    connect(sidebar_, &sidebar_widget_t::workspace_delete_requested, this, [this](const std::string & path) {
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

        rebuild_sidebar();
        rebuild_table();
        scan_workspace();
    });
    connect(sidebar_, &sidebar_widget_t::workspace_save_requested, this, [this](const std::string & path) {
        for (int i = 0; i < workspace_.slot_count(); ++i)
        {
            const auto * slot = workspace_.get_slot(i);
            if (slot && slot->path == path)
            {
                save_dict_encoded(i);
                rebuild_sidebar();
                break;
            }
        }
    });
    connect(sidebar_, &sidebar_widget_t::workspace_file_clicked, this, [this](const std::string & path) {
        for (int i = 0; i < workspace_.slot_count(); ++i)
        {
            const auto * slot = workspace_.get_slot(i);
            if (slot && slot->path == path)
            {
                commit_current_edit();
                workspace_.set_active(i);
                rebuild_table();
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

        commit_current_edit();
        workspace_.set_active(workspace_.slot_count() - 1);
        rebuild_table();
    });

    connect(table_view_, &record_table_view_t::row_selected, this, &main_window_t::on_row_selected);
    connect(table_view_, &record_table_view_t::status_change_requested, this, [this](int row, const QString & new_status) {
        auto * row_data = table_model_->row_at(row);
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

        it->second.records[row_data->chapter_index].status = new_status.toStdString();
        slot->dirty = true;
        set_dirty(true);
        rebuild_table();
    });

    connect(editor_panel_, &editor_panel_t::text_changed, this, &main_window_t::on_translation_changed);
    connect(editor_panel_, &editor_panel_t::apply_clicked, this, [this]() {
        commit_current_edit();
        rebuild_table();
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
    load_config();
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

    save_dict_encoded(active);
    rebuild_sidebar();

    if (!workspace_.has_any_unsaved())
        set_dirty(false);
}

void main_window_t::on_save_all()
{
    commit_current_edit();

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (slot && slot->dirty)
            save_dict_encoded(i);
    }

    rebuild_sidebar();

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

void main_window_t::on_slot_clicked(int slot_index)
{
    if (slot_index == workspace_.get_active_index())
        return;

    commit_current_edit();
    workspace_.set_active(slot_index);
    rebuild_table();
    sidebar_->set_active_slot(slot_index);
}

void main_window_t::on_search_changed(const QString & text)
{
    search_query_ = text;
    rebuild_table();
}

void main_window_t::on_case_sensitive_changed(int /*state*/)
{
    rebuild_table();
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

void main_window_t::rebuild_table()
{
    if (!table_model_)
        return;

    const auto * slot = workspace_.get_active_slot();
    if (!slot)
    {
        table_model_->rebuild({});
        return;
    }

    const bool case_sensitive = case_sensitive_check_ && case_sensitive_check_->isChecked();
    const auto query_lower = search_query_.toLower();

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

    std::vector<table_row_t> rows;
    std::map<tools_t::rec_type_t, size_t> type_counts;
    std::map<std::string, size_t> status_counts;

    for (const auto & [type, chapter] : slot->data)
    {
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            const auto & entry = chapter.records[i];
            type_counts[type]++;
            status_counts[entry.status]++;

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

            if (!search_query_.isEmpty())
            {
                bool match = false;

                if (case_sensitive)
                {
                    const auto q = search_query_.toStdString();
                    match = entry.key_text.find(q) != std::string::npos ||
                            entry.old_text.find(q) != std::string::npos ||
                            entry.new_text.find(q) != std::string::npos;
                }
                else
                {
                    const auto q = query_lower.toStdString();
                    auto contains = [&](const std::string & text)
                    {
                        std::string lower = text;
                        std::transform(lower.begin(), lower.end(), lower.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        return lower.find(q) != std::string::npos;
                    };
                    match = contains(entry.key_text) || contains(entry.old_text) || contains(entry.new_text);
                }

                if (!match)
                    continue;
            }

            rows.push_back({type, entry.key_text, entry.old_text, entry.new_text, entry.status, i});
        }
    }

    table_model_->rebuild(std::move(rows));
    current_row_ = -1;

    size_t total = 0;
    for (const auto & [t, c] : type_counts)
        total += c;

    filter_bar_->update_counts(type_counts);
    filter_bar_->set_total_count(total);
    status_filter_bar_->update_counts(status_counts);
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

    editor_panel_->translation_editor()->setExtraSelections(selections);
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

    const auto new_text_str = current_text.toStdString();
    history_manager_.record_change(row_data->type, row_data->key_text, loaded_text_.toStdString(), new_text_str);

    it->second.records[row_data->chapter_index].new_text = new_text_str;
    it->second.records[row_data->chapter_index].status = "in_progress";
    slot->dirty = true;
    slot->modified_records.insert({row_data->type, row_data->chapter_index});
    set_dirty(true);

    table_model_->update_row(current_row_, new_text_str, "in_progress");
    loaded_text_ = current_text;
    rebuild_sidebar();

    std::map<std::string, size_t> status_counts;
    for (const auto & [type, chapter] : slot->data)
    {
        for (const auto & rec : chapter.records)
            status_counts[rec.status]++;
    }
    status_filter_bar_->update_counts(status_counts);
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

    editor_panel_->original_view()->setPlainText(QString::fromStdString(row_data->old_text));
    editor_panel_->translation_editor()->setPlainText(QString::fromStdString(row_data->new_text));

    const bool is_base = workspace_.is_base_slot(workspace_.get_active_index());
    editor_panel_->translation_editor()->setReadOnly(is_base);

    syntax_hl_original_->set_record_type(row_data->type);

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

    const auto original_text_lower = QString::fromStdString(row_data->old_text).toLower();
    const auto translation_text_lower = QString::fromStdString(row_data->new_text).toLower();

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
    editor_panel_->original_view()->setExtraSelections(orig_selections);

    auto trans_highlights = find_highlights(translation_text_lower, annotations, false);
    QList<QTextEdit::ExtraSelection> trans_selections;
    for (const auto & h : trans_highlights)
    {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(h.is_hyperlink ? QColor(200, 220, 255) : QColor(200, 240, 200));
        sel.cursor = editor_panel_->translation_editor()->textCursor();
        sel.cursor.setPosition(h.start);
        sel.cursor.setPosition(h.start + h.length, QTextCursor::KeepAnchor);
        trans_selections.append(sel);
    }
    editor_panel_->translation_editor()->setExtraSelections(trans_selections);

    if (row_data->status == "adapted" && !adapted_from_str.empty())
    {
        editor_panel_->highlight_adapted_diff(row_data->new_text, adapted_from_str, false);
    }
    else if (row_data->status == "changed" && !adapted_from_str.empty())
    {
        editor_panel_->highlight_adapted_diff(row_data->old_text, adapted_from_str, true);
    }

    const auto history = history_manager_.get_history(row_data->type, row_data->key_text);
    history_panel_->update_history(history, !is_base);

    loaded_text_ = editor_panel_->translation_editor()->toPlainText();
    current_row_ = row;
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
}

void main_window_t::rebuild_sidebar()
{
    const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";

    std::vector<sidebar_section_t> sections;

    sidebar_section_t loaded_section;
    loaded_section.header = "Loaded";

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (!slot)
            continue;

        if (slot->path.find(workspace_dir) == 0)
            continue;

        auto filename = slot->path;
        auto sep = filename.find_last_of("/\\");
        if (sep != std::string::npos)
            filename = filename.substr(sep + 1);

        sidebar_item_t item;
        item.display_name = filename;
        item.path = slot->path;
        item.is_plugin = false;
        item.is_base = workspace_.is_base_slot(i);
        item.is_dirty = slot->dirty;
        item.slot_index = i;
        item.plugin_index = -1;
        loaded_section.items.push_back(item);
    }

    for (int i = 0; i < static_cast<int>(plugin_slots_.size()); ++i)
    {
        const auto & slot = plugin_slots_[i];
        auto sep = slot.path.find_last_of("/\\");
        auto filename = sep != std::string::npos ? slot.path.substr(sep + 1) : slot.path;

        if (!slot.language.empty())
            filename += " [" + slot.language + "]";

        sidebar_item_t item;
        item.display_name = filename;
        item.path = slot.path;
        item.is_plugin = true;
        item.is_base = false;
        item.is_dirty = false;
        item.slot_index = -1;
        item.plugin_index = i;
        loaded_section.items.push_back(item);
    }

    sections.push_back(std::move(loaded_section));

    for (const auto & ws : workspace_sections_)
    {
        sidebar_section_t section;
        section.header = ws.folder_name.empty() ? "Workspace" : ws.folder_name;

        for (int i = 0; i < static_cast<int>(ws.file_names.size()); ++i)
        {
            const auto & path = ws.file_paths[i];
            auto filename = ws.file_names[i];

            auto dot = path.rfind('.');
            auto ext = dot != std::string::npos ? path.substr(dot) : "";
            bool is_plugin_file = (ext == ".esm" || ext == ".esp");

            bool is_base = (filename.find("_BASE_") != std::string::npos);
            bool is_dirty = false;

            int slot_idx = -1;
            for (int j = 0; j < workspace_.slot_count(); ++j)
            {
                const auto * slot = workspace_.get_slot(j);
                if (slot && slot->path == path)
                {
                    slot_idx = j;
                    is_dirty = slot->dirty;
                    break;
                }
            }

            sidebar_item_t item;
            item.display_name = filename;
            item.path = path;
            item.is_plugin = is_plugin_file;
            item.is_base = is_base;
            item.is_dirty = is_dirty;
            item.slot_index = slot_idx;
            item.plugin_index = -1;
            section.items.push_back(item);
        }

        sections.push_back(std::move(section));
    }

    sidebar_->set_sections(sections);
    sidebar_->set_active_slot(workspace_.get_active_index());
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
        spell_hl_->set_spell_checker(nullptr);
        return;
    }

    const auto lang_name = spell_lang_combo_->itemText(index);
    const auto app_dir = QCoreApplication::applicationDirPath();
    const auto aff_path = app_dir + "/dictionaries/" + lang_name + ".aff";
    const auto dic_path = app_dir + "/dictionaries/" + lang_name + ".dic";

    if (!spell_checker_.load(aff_path.toStdString(), dic_path.toStdString()))
        return;

    spell_hl_->set_spell_checker(&spell_checker_);
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

    for (const auto & p : config_.plugin_paths)
    {
        if (!QFile::exists(QString::fromStdString(p)))
            continue;

        plugin_slot_t slot;
        slot.path = p;
        detect_plugin_info(slot);
        plugin_slots_.push_back(std::move(slot));
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

    const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";

    for (int i = 0; i < workspace_.slot_count(); ++i)
    {
        const auto * slot = workspace_.get_slot(i);
        if (!slot)
            continue;

        if (slot->path.find(workspace_dir) == 0)
            continue;

        if (workspace_.is_user_slot(i))
            config_.user_dict_paths.push_back(slot->path);
        else
            config_.base_dict_paths.push_back(slot->path);
    }

    config_.plugin_paths.clear();
    for (const auto & slot : plugin_slots_)
        config_.plugin_paths.push_back(slot.path);

    const auto path = QCoreApplication::applicationDirPath() + "/yampt_gui.ini";
    config_.save(path.toStdString());
}

void main_window_t::on_load_plugin()
{
    const auto paths = QFileDialog::getOpenFileNames(
        this, "Load Plugin", "", "ESM/ESP Files (*.esm *.esp);;All Files (*.*)");

    if (paths.isEmpty())
        return;

    for (const auto & path : paths)
    {
        plugin_slot_t slot;
        slot.path = path.toStdString();
        detect_plugin_info(slot);
        plugin_slots_.push_back(std::move(slot));
    }

    rebuild_sidebar();
}

void main_window_t::on_plugin_operation(int plugin_index, plugin_op_t op)
{
    std::string plugin_path;
    std::string workspace_folder;

    if (plugin_index < 0)
    {
        int ws_flat_index = -(plugin_index + 1);

        int offset = 0;
        bool found = false;

        for (const auto & section : workspace_sections_)
        {
            for (int i = 0; i < static_cast<int>(section.file_paths.size()); ++i)
            {
                if (offset == ws_flat_index)
                {
                    plugin_path = section.file_paths[i];
                    workspace_folder = section.folder_name;
                    found = true;
                    break;
                }
                ++offset;
            }
            if (found)
                break;
        }

        if (!found)
            return;
    }
    else
    {
        if (plugin_index >= static_cast<int>(plugin_slots_.size()))
            return;

        plugin_path = plugin_slots_[plugin_index].path;
    }
    const auto encoding = get_current_tools_encoding();

    std::string output_dir;
    if (!workspace_folder.empty())
    {
        output_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/" + workspace_folder + "/";
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
        std::vector<int> plugin_indices;
        int best_match_row = 0;

        for (int i = 0; i < static_cast<int>(plugin_slots_.size()); ++i)
        {
            if (i == plugin_index)
                continue;

            const auto & slot = plugin_slots_[i];
            auto sep = slot.path.find_last_of("/\\");
            auto name = sep != std::string::npos ? slot.path.substr(sep + 1) : slot.path;

            auto name_lower = name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (name_lower == source_lower && slot.path != plugin_path)
                best_match_row = static_cast<int>(plugin_names.size());

            if (!slot.language.empty())
                name += " [" + slot.language + "]";

            plugin_names.append(QString::fromStdString(name));
            plugin_indices.push_back(i);
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

        const auto & native_slot = plugin_slots_[plugin_indices[sel_idx]];
        const auto & foreign_lang = (plugin_index >= 0 && plugin_index < static_cast<int>(plugin_slots_.size()))
            ? plugin_slots_[plugin_index].language : std::string{};
        result = executor_.make_base(plugin_path, native_slot.path, foreign_lang, native_slot.language);
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

void main_window_t::on_plugin_unload(int plugin_index)
{
    if (plugin_index < 0 || plugin_index >= static_cast<int>(plugin_slots_.size()))
        return;

    plugin_slots_.erase(plugin_slots_.begin() + plugin_index);
    rebuild_sidebar();
}

void main_window_t::detect_plugin_info(plugin_slot_t & slot)
{
    auto sep = slot.path.find_last_of("/\\");
    auto filename = sep != std::string::npos ? slot.path.substr(sep + 1) : slot.path;

    auto lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    slot.is_master = (lower == "morrowind.esm" || lower == "tribunal.esm" || lower == "bloodmoon.esm");

    if (!slot.is_master)
        return;

    std::error_code ec;
    auto file_size = std::filesystem::file_size(slot.path, ec);
    if (ec)
        return;

    struct known_master_t
    {
        uintmax_t size;
        const char * language;
    };

    static const known_master_t known_masters[] = {
        {79837557, "EN"}, {9631798, "EN"}, {4565686, "EN"},
        {80640776, "DE"}, {9797295, "DE"}, {6069165, "DE"},
        {80105097, "PL"}, {9658076, "PL"}, {4626565, "PL"},
        {80681814, "FR"}, {10015689, "FR"}, {4697358, "FR"},
        {79857000, "RU"}, {9702000, "RU"}, {4625000, "RU"},
    };

    for (const auto & m : known_masters)
    {
        if (file_size == m.size)
        {
            slot.language = m.language;
            return;
        }
    }
}

void main_window_t::scan_workspace()
{
    const auto app_dir = QCoreApplication::applicationDirPath();
    QDir workspace_dir(app_dir + "/workspace");

    std::vector<workspace_scan_section_t> sections;

    if (!workspace_dir.exists())
    {
        workspace_sections_ = sections;
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

    workspace_scan_section_t root_section;
    root_section.folder_name = "";
    const auto root_files = workspace_dir.entryList({"*.json", "*.xml", "*.esm", "*.esp"}, QDir::Files);
    for (const auto & f : root_files)
    {
        root_section.file_names.push_back(f.toStdString());
        root_section.file_paths.push_back(workspace_dir.absoluteFilePath(f).toStdString());
    }
    if (!root_section.file_names.empty())
        sections.push_back(std::move(root_section));

    const auto subdirs = workspace_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto & sub : subdirs)
    {
        QDir sub_dir(workspace_dir.absoluteFilePath(sub));
        const auto sub_files = sub_dir.entryList({"*.json", "*.xml", "*.esm", "*.esp"}, QDir::Files);
        if (sub_files.isEmpty())
            continue;

        workspace_scan_section_t section;
        section.folder_name = sub.toStdString();
        for (const auto & f : sub_files)
        {
            section.file_names.push_back(f.toStdString());
            section.file_paths.push_back(sub_dir.absoluteFilePath(f).toStdString());
        }
        sections.push_back(std::move(section));
    }

    for (const auto & section : sections)
    {
        for (const auto & path : section.file_paths)
        {
            auto dot = path.rfind('.');
            if (dot == std::string::npos)
                continue;

            auto ext = path.substr(dot);
            if (ext != ".json" && ext != ".xml")
                continue;

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
    }

    workspace_sections_ = sections;
    rebuild_sidebar();

    std::vector<dict_source_t> sources;
    for (const auto & dict_slot : workspace_.get_all_slots())
        sources.push_back({&dict_slot.data, dict_slot.path});
    annotation_manager_.rebuild(sources);
}

std::vector<dict_selection_dialog_t::dict_entry_t> main_window_t::build_dict_entries(const std::string & workspace_folder) const
{
    std::vector<dict_selection_dialog_t::dict_entry_t> entries;
    std::set<std::string> added_paths;

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
        added_paths.insert(slot->path);
    }

    for (const auto & section : workspace_sections_)
    {
        for (int i = 0; i < static_cast<int>(section.file_paths.size()); ++i)
        {
            const auto & path = section.file_paths[i];

            if (added_paths.count(path))
                continue;

            auto dot = path.rfind('.');
            if (dot == std::string::npos)
                continue;

            auto ext = path.substr(dot);
            if (ext != ".json" && ext != ".xml")
                continue;

            const auto & filename = section.file_names[i];
            auto kind = (filename.find("_BASE_") != std::string::npos) ? dict_kind_t::base : dict_kind_t::user;

            entries.push_back({filename, path, kind, false});
        }
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
