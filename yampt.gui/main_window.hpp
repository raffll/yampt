#pragma once

#include "annotation_manager.hpp"
#include "dict_selection_dialog.hpp"
#include "dict_workspace.hpp"
#include "editor_config.hpp"
#include "encoding_utils.hpp"
#include "grammar_checker.hpp"
#include "history_manager.hpp"
#include "operation_executor.hpp"
#include "record_table_model.hpp"
#include "search_engine.hpp"
#include "sidebar_widget.hpp"
#include "spell_checker.hpp"
#include "validation_manager.hpp"
#include "yaml_l10n_reader.hpp"

#include "../yampt/file_list.hpp"

#include <QMainWindow>
#include <QTextEdit>

class QFileSystemWatcher;
class QTimer;

class annotations_panel_t;
class book_preview_t;
class composite_highlighter_t;
class editor_panel_t;
class editor_text_edit_t;
class filter_tree_t;
class find_replace_dialog_t;
class history_panel_t;
class log_tab_t;
class record_table_view_t;
class spell_context_menu_t;
class status_filter_bar_t;
class validation_indicator_t;

class QAction;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QSplitter;
class QTabWidget;

struct extra_selections_state_t
{
    QList<QTextEdit::ExtraSelection> annotations;
    QList<QTextEdit::ExtraSelection> grammar;
    QList<QTextEdit::ExtraSelection> adapted_diff;
};

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
    void on_unload_slot(int index);
    void on_plugin_operation(const std::string & plugin_path, plugin_op_t op);
    void on_plugin_unload(const std::string & path);
    void on_find();
    void on_next_search();
    void on_prev_search();
    void on_refresh();
    void on_escape();
    void on_search_changed(const QString & text);
    void on_case_sensitive_changed(int state);
    void on_row_selected(int row);
    void on_translation_changed();
    void on_whitespace_toggled(bool checked);
    void on_encoding_changed(int index);
    void on_filters_changed();
    void on_status_filters_changed();

    void on_item_clicked(const std::string & path);
    void on_operation_requested(const std::string & path, plugin_op_t op);
    void on_save_requested(const std::string & path);
    void on_unload_requested(const std::string & path);
    void on_delete_requested(const std::string & path);

private:
    void rebuild_table();
    void commit_current_edit();
    void load_record(int row);
    void save_config();
    void load_config();
    void rebuild_sidebar();
    void save_dict_encoded(int slot_index);
    void update_annotations();
    void update_validation();
    void update_status_counts();
    void scan_spell_dictionaries();
    void on_spell_lang_changed(int index);
    void scan_workspace();
    void update_watcher_paths();
    std::vector<dict_selection_dialog_t::dict_entry_t> build_dict_entries(const std::string & source_dir = {}) const;
    void ensure_dicts_loaded(const std::vector<std::string> & paths);
    tools_t::encoding_t get_current_tools_encoding() const;
    void apply_extra_selections(editor_text_edit_t * editor, const extra_selections_state_t & state);
    int propagate_translation(const std::string & old_text, const std::string & new_text);

    QAction * add_folder_action_ = nullptr;
    QAction * import_archive_action_ = nullptr;
    QAction * save_action_ = nullptr;
    QAction * save_all_action_ = nullptr;
    QAction * find_action_ = nullptr;
    QAction * next_search_action_ = nullptr;
    QAction * prev_search_action_ = nullptr;
    QAction * refresh_action_ = nullptr;
    QAction * escape_action_ = nullptr;
    QAction * find_replace_action_ = nullptr;

    QAction * sidebar_toggle_ = nullptr;
    QAction * bottom_panel_toggle_ = nullptr;

    QSplitter * central_splitter_ = nullptr;

    QSplitter * left_splitter_ = nullptr;
    QSplitter * right_splitter_ = nullptr;
    QTabWidget * left_tabs_ = nullptr;
    QTabWidget * info_tabs_ = nullptr;
    QTabWidget * record_tabs_ = nullptr;

    QLabel * search_label_ = nullptr;
    QLineEdit * search_field_ = nullptr;
    QCheckBox * case_sensitive_check_ = nullptr;
    QCheckBox * regex_check_ = nullptr;
    QCheckBox * search_col_key_ = nullptr;
    QCheckBox * search_col_original_ = nullptr;
    QCheckBox * search_col_translation_ = nullptr;
    QComboBox * encoding_combo_ = nullptr;
    QComboBox * spell_lang_combo_ = nullptr;
    QCheckBox * grammar_check_ = nullptr;
    QCheckBox * whitespace_check_ = nullptr;

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

    filter_tree_t * filter_tree_ = nullptr;
    status_filter_bar_t * status_filter_bar_ = nullptr;
    record_table_view_t * table_view_ = nullptr;
    sidebar_widget_t * sidebar_ = nullptr;
    book_preview_t * book_preview_ = nullptr;
    QLabel * active_file_label_ = nullptr;
    QLabel * progress_label_ = nullptr;
    validation_indicator_t * validation_indicator_ = nullptr;
    composite_highlighter_t * hl_original_ = nullptr;
    composite_highlighter_t * hl_adapted_ = nullptr;
    composite_highlighter_t * hl_translation_ = nullptr;

    grammar_checker_t grammar_checker_;
    search_engine_t search_engine_;
    spell_context_menu_t * spell_menu_ = nullptr;
    annotations_panel_t * annotations_panel_ = nullptr;
    history_panel_t * history_panel_ = nullptr;

    annotation_manager_t annotation_manager_;
    history_manager_t history_manager_;
    spell_checker_t spell_checker_;
    validation_manager_t validation_manager_;

    dict_workspace_t workspace_;
    file_list_t file_list_;
    codepage_t current_codepage_ = codepage_t::windows_1252;
    editor_config_t config_;

    log_tab_t * log_tab_ = nullptr;
    operation_executor_t executor_;

    extra_selections_state_t extra_sel_original_;
    extra_selections_state_t extra_sel_adapted_;
    extra_selections_state_t extra_sel_translation_;

    find_replace_dialog_t * find_replace_dialog_ = nullptr;

    QFileSystemWatcher * fs_watcher_ = nullptr;
    QTimer * rescan_timer_ = nullptr;

    std::string lua_active_path_;
    std::vector<l10n_entry_t> lua_entries_;
    std::set<size_t> lua_modified_indices_;

    void save_lua_temp();
};
