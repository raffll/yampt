#pragma once

#include "controller/glossary.hpp"
#include "dialog/dict_selection_dialog.hpp"
#include "model/dict_document.hpp"
#include "model/document.hpp"
#include "io/editor_config.hpp"
#include "controller/editor_controller.hpp"
#include <io/codepage.hpp>
#include "controller/find_replace.hpp"
#include "controller/grammar_checker.hpp"
#include "controller/edit_history.hpp"
#include "controller/operation_executor.hpp"
#include "model/plugin_op.hpp"
#include "model/record_table_model.hpp"
#include "controller/row_filter.hpp"
#include "session.hpp"
#include "model/sidebar_model.hpp"
#include "view/sidebar_view.hpp"
#include "utility/spell_checker.hpp"
#include "view/table_view.hpp"
#include "view/translation_suggestion_view.hpp"
#include "controller/byte_limit_validator.hpp"

#include <model/file_list.hpp>

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
class composite_highlighter_t;
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
class QActionGroup;
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

	QAction * add_folder_action_ = nullptr;
	QAction * import_archive_action_ = nullptr;
	QAction * save_action_ = nullptr;
	QAction * save_all_action_ = nullptr;
	QAction * quit_action_ = nullptr;
	QAction * find_action_ = nullptr;
	QAction * escape_action_ = nullptr;

	QToolBar * toolbar_ = nullptr;

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
	QPushButton * case_sensitive_check_ = nullptr;
	QPushButton * regex_check_ = nullptr;
	QPushButton * search_col_key_ = nullptr;
	QPushButton * search_col_original_ = nullptr;
	QPushButton * search_col_translation_ = nullptr;
	QActionGroup * encoding_group_ = nullptr;
	QMenu * spelling_menu_ = nullptr;
	QActionGroup * spelling_group_ = nullptr;
	QAction * grammar_check_ = nullptr;
	QAction * whitespace_check_ = nullptr;

	QString search_query_;

	bool has_unsaved_changes_ = false;

	std::set<tools_t::rec_type_t> type_filter_;
	std::set<status_t> status_filter_;
	bool type_filter_solo_ = false;

	record_table_model_t * table_model_ = nullptr;
	editor_view_t * editor_view_ = nullptr;

	filter_tree_view_t * filter_tree_view_ = nullptr;
	status_filter_view_t * status_filter_view_ = nullptr;
	record_table_view_t * table_view_ = nullptr;
	sidebar_view_t * sidebar_ = nullptr;
	book_preview_view_t * book_preview_view_ = nullptr;
	QLabel * active_file_label_ = nullptr;
	QLabel * progress_label_ = nullptr;
	validation_view_t * validation_view_ = nullptr;
	composite_highlighter_t * hl_original_ = nullptr;
	composite_highlighter_t * hl_adapted_ = nullptr;
	composite_highlighter_t * hl_translation_ = nullptr;

	grammar_checker_t grammar_checker_;
	row_filter_t row_filter_;
	spell_context_menu_t * spell_menu_ = nullptr;
	annotations_view_t * annotations_view_ = nullptr;
	history_view_t * history_view_ = nullptr;
	translation_suggestion_view_t * translation_tab_ = nullptr;

	glossary_t glossary_;
	edit_history_t edit_history_;
	spell_checker_t spell_checker_;
	byte_limit_validator_t byte_limit_validator_;
	editor_controller_t editor_controller_;

	file_list_t file_list_;
	codepage_t current_codepage_ = codepage_t::windows_1252;
	session_t session_;
	editor_config_t config_;
	std::unordered_map<std::string, filter_state_t> filter_states_;
	size_t last_annotation_version_ = 0;

	log_view_t * log_view_ = nullptr;
	operation_executor_t executor_;

	std::unique_ptr<table_view_t> table_display_;

	extra_selections_state_t extra_sel_original_;
	extra_selections_state_t extra_sel_adapted_;
	extra_selections_state_t extra_sel_translation_;

	find_replace_dialog_t * find_replace_dialog_ = nullptr;
	find_replace_t * find_replace_ = nullptr;

	QFileSystemWatcher * fs_watcher_ = nullptr;
	QTimer * rescan_timer_ = nullptr;

	QMenu * translator_file_menu_ = nullptr;
	QMenu * translator_view_menu_ = nullptr;

	document_t * active_doc_ = nullptr;
};
