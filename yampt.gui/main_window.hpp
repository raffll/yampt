#pragma once

#include "annotation_manager.hpp"
#include "dict_workspace.hpp"
#include "editor_config.hpp"
#include "encoding_utils.hpp"
#include "history_manager.hpp"
#include "record_table_model.hpp"
#include "spell_checker.hpp"
#include "syntax_highlighter.hpp"
#include "validation_manager.hpp"

#include <QMainWindow>

class annotation_highlighter_t;
class annotations_panel_t;
class book_preview_t;
class editor_panel_t;
class filter_bar_t;
class history_panel_t;
class hyperlink_highlighter_t;
class mwscript_highlighter_t;
class record_table_view_t;
class sidebar_widget_t;
class spell_check_highlighter_t;
class spell_context_menu_t;
class status_filter_bar_t;
class validation_indicator_t;

class QAction;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLineEdit;
class QSplitter;
class QTabWidget;

class main_window_t : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window_t(QWidget * parent = nullptr);

    void set_dirty(bool dirty);

protected:
    void closeEvent(QCloseEvent * event) override;

private slots:
    void on_save();
    void on_save_all();
    void on_open();
    void on_open_user_dict();
    void on_open_base_dict();
    void on_unload_slot(int index);
    void on_find();
    void on_next_search();
    void on_prev_search();
    void on_refresh();
    void on_escape();
    void on_search_changed(const QString & text);
    void on_case_sensitive_changed(int state);
    void on_row_selected(int row);
    void on_translation_changed();
    void on_slot_clicked(int slot_index);
    void on_whitespace_toggled(bool checked);
    void on_encoding_changed(int index);
    void on_filters_changed();
    void on_status_filters_changed();

private:
    void rebuild_table();
    void commit_current_edit();
    void load_record(int row);
    void save_config();
    void load_config();
    void rebuild_sidebar();
    void update_annotations();
    void update_validation();
    void scan_spell_dictionaries();
    void on_spell_lang_changed(int index);

    QAction * save_action_ = nullptr;
    QAction * save_all_action_ = nullptr;
    QAction * open_action_ = nullptr;
    QAction * open_base_action_ = nullptr;
    QAction * find_action_ = nullptr;
    QAction * next_search_action_ = nullptr;
    QAction * prev_search_action_ = nullptr;
    QAction * refresh_action_ = nullptr;
    QAction * escape_action_ = nullptr;

    QAction * sidebar_toggle_ = nullptr;
    QAction * bottom_panel_toggle_ = nullptr;
    QAction * whitespace_toggle_ = nullptr;

    QSplitter * central_splitter_ = nullptr;

    QSplitter * left_splitter_ = nullptr;
    QSplitter * right_splitter_ = nullptr;
    QTabWidget * info_tabs_ = nullptr;
    QTabWidget * record_tabs_ = nullptr;

    QLineEdit * search_field_ = nullptr;
    QCheckBox * case_sensitive_check_ = nullptr;
    QComboBox * encoding_combo_ = nullptr;
    QComboBox * spell_lang_combo_ = nullptr;

    QString search_query_;

    bool dirty_ = false;
    bool loading_record_ = false;
    int current_row_ = -1;
    QString loaded_text_;

    std::set<tools_t::rec_type_t> type_filter_;
    std::set<std::string> status_filter_;
    bool type_filter_solo_ = false;

    record_table_model_t * table_model_ = nullptr;
    editor_panel_t * editor_panel_ = nullptr;

    filter_bar_t * filter_bar_ = nullptr;
    status_filter_bar_t * status_filter_bar_ = nullptr;
    record_table_view_t * table_view_ = nullptr;
    sidebar_widget_t * sidebar_ = nullptr;
    book_preview_t * book_preview_ = nullptr;
    validation_indicator_t * validation_indicator_ = nullptr;
    mwscript_highlighter_t * syntax_hl_original_ = nullptr;
    mwscript_highlighter_t * syntax_hl_translation_ = nullptr;
    spell_check_highlighter_t * spell_hl_ = nullptr;
    annotation_highlighter_t * annotation_hl_ = nullptr;
    hyperlink_highlighter_t * hyperlink_hl_ = nullptr;
    spell_context_menu_t * spell_menu_ = nullptr;
    annotations_panel_t * annotations_panel_ = nullptr;
    history_panel_t * history_panel_ = nullptr;

    annotation_manager_t annotation_manager_;
    history_manager_t history_manager_;
    spell_checker_t spell_checker_;
    validation_manager_t validation_manager_;

    dict_workspace_t workspace_;
    codepage_t current_codepage_ = codepage_t::windows_1252;
    editor_config_t config_;
};
