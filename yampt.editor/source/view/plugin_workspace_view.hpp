#pragma once

#include "../model/nav_tree_model.hpp"
#include "../model/plugin_session.hpp"
#include "messages_view.hpp"
#include "nav_tree_view.hpp"
#include "preview_view.hpp"
#include "record_view.hpp"
#include <scanner/plugin_scan.hpp>
#include <set>
#include <QLabel>
#include <QSplitter>
#include <QTabWidget>
#include <QWidget>

class QDragMoveEvent;
class QDropEvent;
class settings_store_t;

class plugin_workspace_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit plugin_workspace_view_t(settings_store_t & settings, QWidget * parent = nullptr);

	void save_session_state();
	void restore_session_state();

	void set_conflicts_only(bool value);
	void set_show_positions(bool value);
	void set_show_deleted_strikeout(bool value);

	QLabel * count_label() const
	{
		return m_lbl_count;
	}

	QLabel * status_label() const
	{
		return m_status_label;
	}

	void refresh_views();

public slots:
	void on_load_data_files();
	void on_load_mo2_profile();
	void on_load_openmw_cfg();
	void on_save_plugin();
	void on_unload_all();
	void on_create_merged_patch();
	void on_advanced_filter();
	void set_hide_duplicates(bool hide);

private slots:
	void on_nav_selection_changed(const nav_tree_model_t::node_info_t & info);
	void on_nav_context_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info);
	void on_filter_changed();
	void on_remove_itm();
	void on_view_context_menu(const QPoint & global_pos, const QModelIndex & index);
	void on_view_copy();
	void on_view_selection_changed(const QModelIndex & current);

private:
	void setup_views();
	void setup_connections();
	void rebuild_after_load();
	void update_status();
	void log_message(const std::string & msg);
	void rebuild_nav_preserving_state();
	void load_plugins_from_paths(const std::vector<std::string> & paths, const std::string & base_path);
	void load_existing_merged_patch();
	std::string resolve_merge_output_path() const;
	void save_merged_patch();
	bool save_merge_to_file(const std::string & output_path, const std::string & author, const std::string & description);
	void display_record_in_view(const conflict_entry_t & entry);
	bool handle_subrecord_drop(QDropEvent * drop_event);

	void copy_sub_record_to_merge(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    const std::string & sub_type,
	    int binary_idx);
	void copy_group_to_merge(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    int group_row_idx);
	void copy_field_to_merge(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    const std::string & sub_type,
	    size_t sub_size,
	    int binary_idx,
	    int field_idx);
	std::string read_source_content(int plugin_idx, const std::string & rec_type, const std::string & record_id);
	std::string ensure_merge_record(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    const std::string & source_content);
	int find_plugin_column(int plugin_idx) const;

	int create_merge_records();
	bool eventFilter(QObject * obj, QEvent * event) override;
	bool handle_drag_move_view(QDragMoveEvent * drag);
	bool handle_drag_move_nav(QDragMoveEvent * drag);
	bool handle_drop_on_view(QDropEvent * drop_event);
	bool handle_drop_on_nav(QDropEvent * drop_event);
	void refresh_after_merge(const std::string & rec_type, const std::string & record_id);

	settings_store_t & m_settings;
	plugin_session_t * m_session = nullptr;

	bool m_conflicts_only = false;
	QLabel * m_lbl_count = nullptr;

	QSplitter * m_main_splitter = nullptr;
	QSplitter * m_content_splitter = nullptr;
	nav_tree_view_t * m_nav_view = nullptr;
	record_view_t * m_record_view = nullptr;
	messages_view_t * m_messages = nullptr;
	preview_view_t * m_preview = nullptr;
	QTabWidget * m_bottom_tabs = nullptr;

	QLabel * m_status_label = nullptr;

	bool m_filter_active = false;
	nav_tree_model_t::filter_state_t m_last_filter_state;

	bool m_has_filter_active = false;
	nav_tree_model_t::filter_state_t m_last_quick_filter;
	bool m_hide_duplicates = false;
};
