#pragma once

#include "editor/byte_limit_validator.hpp"
#include "editor/edit_history.hpp"
#include "editor/editor_controller.hpp"
#include "editor/find_replace.hpp"
#include "editor/glossary.hpp"
#include "editor/grammar_checker.hpp"
#include "editor/operation_executor.hpp"
#include "editor/row_filter.hpp"
#include "dialog/dict_selection_dialog.hpp"
#include "io/app_settings.hpp"
#include "model/dict_document.hpp"
#include "model/document.hpp"
#include "model/plugin_op.hpp"
#include "model/record_table_model.hpp"
#include "model/sidebar_model.hpp"
#include "session.hpp"
#include "utility/spell_checker.hpp"
#include "view/sidebar_view.hpp"
#include "view/table_view.hpp"
#include "view/translation_suggestion_view.hpp"
#include <io/codepage.hpp>
#include <io/file_list.hpp>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <QMainWindow>
#include <QStringList>
#include <QTextEdit>

class QFileSystemWatcher;
class QTimer;

class annotations_view_t;
class book_preview_view_t;
class editor_highlighter_t;
class editor_view_t;
class translation_edit_view_t;
class filter_tree_view_t;
class find_replace_dialog_t;
class history_view_t;
class log_view_t;
class record_table_view_t;
class spell_context_menu_t;
class status_filter_view_t;
class validation_view_t;

class QAction;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QSplitter;
class QTabWidget;
class QToolBar;

struct filter_state_t
{
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;
	bool type_filter_solo = false;
};

struct make_base_params_t
{
	std::string native_path;
	std::string foreign_lang;
	std::string native_lang;
	base_mode_t base_mode;
	std::string dictionary_aff_path;
};

struct annotation_highlight_t
{
	int start;
	int length;
	bool is_hyperlink;
};

enum class highlight_sort_policy_t
{
	length_first,
	hyperlink_first,
};

struct highlight_config_t
{
	const std::vector<annotation_t> * annotations;
	bool use_old_text;
	highlight_sort_policy_t sort_policy;
};

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

	void set_unsaved_changes(bool dirty);

protected:
	void closeEvent(QCloseEvent * event) override;

private slots:
	void on_save();
	void on_save_all();
	void on_merge();
	void on_plugin_operation(const std::string & plugin_path, plugin_op_t op);
	void on_plugin_unload(const std::string & path);
	void on_find();
	void on_escape();
	void on_search_changed(const QString & text);
	void on_case_sensitive_changed(int state);
	void on_row_selected(int row);
	void on_translation_changed();
	void on_whitespace_toggled(bool checked);
	void on_encoding_changed(int index);
	void on_filters_changed();
	void on_status_filters_changed();
	void on_open_settings();
	void on_settings_applied(const std::string & category);

	void on_item_clicked(const std::string & path);
	void on_operation_requested(const std::string & path, plugin_op_t op);
	void on_save_requested(const std::string & path);
	void on_unload_requested(const std::string & path);
	void on_delete_requested(const std::string & path);

private:
	void setup_central_widget();
	void setup_menu_bar();
	void setup_toolbar();
	void setup_sidebar();
	void setup_editor_panel();
	void setup_status_bar();
	void setup_table_display();

	void connect_sidebar_signals();
	void connect_editor_signals();
	void connect_search_signals();
	void connect_menu_signals();

	void switch_document(document_t * new_doc);
	void clear_editor_panels();
	void rebuild_table();
	void commit_current_edit();
	void load_record(int row);
	void save_config();
	void load_config();
	void rebuild_sidebar();
	void rebuild_annotations();
	void update_sidebar_item(const std::string & path);
	void save_current_filter_state();
	void restore_filter_state(const std::string & path);
	void update_annotations();
	void update_validation();
	void update_status_counts();
	void scan_spell_dictionaries();
	void on_spell_lang_changed(int index);
	void scan_workspace();
	void update_watcher_paths();
	void register_shortcuts();
	void shortcut_copy_original();
	void shortcut_commit_status(status_t new_status);
	std::vector<dict_selection_dialog_t::dict_entry_t> build_dict_entries(const std::string & source_dir = {}) const;
	void apply_extra_selections(translation_edit_view_t * editor, const extra_selections_state_t & state);

	// rebuild_table helpers
	void rebuild_table_yaml(document_t * target_doc);
	void rebuild_table_dict(dict_document_t * dict_doc);

	// load_record helpers
	void load_record_clear(int row);
	void load_record_script(const table_row_t * row_data);
	void load_record_plain(const table_row_t * row_data);
	std::vector<annotation_highlight_t> find_annotation_highlights(
	    const QString & text_lower,
	    const highlight_config_t & config);
	QList<QTextEdit::ExtraSelection> build_highlight_selections(
	    translation_edit_view_t * target_editor,
	    const std::vector<annotation_highlight_t> & highlights);

	// on_translation_changed helpers
	void apply_translation_highlights(const table_row_t * row_data);

	// commit_current_edit helpers
	void commit_dict_edit(dict_document_t * dict_doc, const table_row_t * row_data, const std::string & new_text_str);
	void commit_yaml_edit(document_t * target_doc, const table_row_t * row_data, const std::string & new_text_str);
	void sync_propagated_rows(dict_document_t * dict_doc);

	// on_plugin_operation helpers
	std::optional<make_base_params_t> show_make_base_dialog(const std::string & plugin_path);
	void start_batch_translation(dict_document_t * dict_doc);
	void log_operation_result(
	    const std::string & plugin_path,
	    plugin_op_t op_type,
	    const operation_executor_t::result_t & result);

	// update_watcher_paths helper
	void add_directory_recursive(QStringList & target_paths, const QString & directory);

	QAction * m_add_folder_action = nullptr;
	QAction * m_import_archive_action = nullptr;
	QAction * m_merge_action = nullptr;
	QAction * m_save_action = nullptr;
	QAction * m_save_all_action = nullptr;
	QAction * m_quit_action = nullptr;
	QAction * m_find_action = nullptr;
	QAction * m_escape_action = nullptr;
	QAction * m_settings_action = nullptr;

	QAction * m_copy_original_action = nullptr;
	QAction * m_set_in_progress_action = nullptr;
	QAction * m_set_translated_action = nullptr;

	QToolBar * m_toolbar = nullptr;

	QAction * m_sidebar_toggle = nullptr;
	QAction * m_bottom_panel_toggle = nullptr;

	QSplitter * m_central_splitter = nullptr;

	QSplitter * m_left_splitter = nullptr;
	QSplitter * m_right_splitter = nullptr;
	QTabWidget * m_left_tabs = nullptr;
	QTabWidget * m_info_tabs = nullptr;
	QTabWidget * m_record_tabs = nullptr;

	QLabel * m_search_label = nullptr;
	QLineEdit * m_search_field = nullptr;
	QPushButton * m_case_sensitive_check = nullptr;
	QPushButton * m_regex_check = nullptr;
	QPushButton * m_search_col_key = nullptr;
	QPushButton * m_search_col_original = nullptr;
	QPushButton * m_search_col_translation = nullptr;
	QAction * m_grammar_check = nullptr;
	QAction * m_whitespace_check = nullptr;

	QString m_search_query;

	bool m_has_unsaved_changes = false;

	std::set<tools_t::rec_type_t> m_type_filter;
	std::set<status_t> m_status_filter;
	bool m_type_filter_solo = false;

	record_table_model_t * m_table_model = nullptr;
	editor_view_t * m_editor_view = nullptr;

	filter_tree_view_t * m_filter_tree_view = nullptr;
	status_filter_view_t * m_status_filter_view = nullptr;
	record_table_view_t * m_table_view = nullptr;
	sidebar_view_t * m_sidebar = nullptr;
	book_preview_view_t * m_book_preview_view = nullptr;
	QLabel * m_active_file_label = nullptr;
	QLabel * m_progress_label = nullptr;
	validation_view_t * m_validation_view = nullptr;
	editor_highlighter_t * m_hl_original = nullptr;
	editor_highlighter_t * m_hl_adapted = nullptr;
	editor_highlighter_t * m_hl_translation = nullptr;

	grammar_checker_t m_grammar_checker;
	row_filter_t m_row_filter;
	spell_context_menu_t * m_spell_menu = nullptr;
	annotations_view_t * m_annotations_view = nullptr;
	history_view_t * m_history_view = nullptr;
	translation_suggestion_view_t * m_translation_tab = nullptr;

	glossary_t m_glossary;
	edit_history_t m_edit_history;
	spell_checker_t m_spell_checker;
	byte_limit_validator_t m_byte_limit_validator;
	editor_controller_t m_editor_controller;

	file_list_t m_file_list;
	codepage_t m_current_codepage = codepage_t::windows_1252;
	session_t m_session;
	app_settings_t m_settings{"yTranslator.ini"};
	std::unordered_map<std::string, filter_state_t> m_filter_states;
	size_t m_last_annotation_version = 0;

	log_view_t * m_log_view = nullptr;
	operation_executor_t m_executor;

	std::unique_ptr<table_view_t> m_table_display;

	extra_selections_state_t m_extra_sel_original;
	extra_selections_state_t m_extra_sel_adapted;
	extra_selections_state_t m_extra_sel_translation;

	find_replace_dialog_t * m_find_replace_dialog = nullptr;
	find_replace_t * m_find_replace = nullptr;

	QFileSystemWatcher * m_fs_watcher = nullptr;
	QTimer * m_rescan_timer = nullptr;

	QMenu * m_translator_file_menu = nullptr;
	QMenu * m_translator_view_menu = nullptr;

	document_t * m_active_doc = nullptr;
};
