#pragma once

#include "../controller/merge_controller.hpp"
#include "../controller/view_context_menu.hpp"
#include "../model/nav_tree_model.hpp"
#include "../session/plugin_session.hpp"
#include "messages_view.hpp"
#include "nav_tree_view.hpp"
#include "preview_view.hpp"
#include "record_view.hpp"
#include <scanner/plugin_scan.hpp>
#include <scanner/lua_scanner.hpp>
#include <QLabel>
#include <QSplitter>
#include <QTabWidget>
#include <QWidget>

class lua_conflicts_view_t;
class lua_scan_worker_t;
class settings_store_t;

class plugin_workspace_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit plugin_workspace_view_t(settings_store_t & settings, QWidget * parent = nullptr);

	void save_session_state();
	void restore_session_state();

	void set_conflicts_only(bool value);
	void set_show_deleted_strikeout(bool value);

	bool is_conflicts_only() const
	{
		return m_conflicts_only;
	}

	bool is_hide_duplicates() const
	{
		return m_hide_duplicates;
	}

	bool is_show_deleted_strikeout() const;

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
	void on_unload_all();
	void on_create_merged_patch();
	void on_clean_all();
	void on_advanced_filter();
	void on_settings_changed();
	void set_hide_duplicates(bool hide);

private slots:
	void on_nav_selection_changed(const nav_tree_model_t::node_info_t & info);
	void on_nav_context_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info);
	void on_filter_changed();
	void on_view_context_menu(const QPoint & global_pos, const QModelIndex & index);
	void on_view_copy();
	void on_view_selection_changed(const QModelIndex & current);

private:
	void setup_views();
	void setup_connections();
	void rebuild_after_load();
	void apply_user_conflict_rules();
	void update_status();
	void log_message(const std::string & msg);
	void rebuild_nav_preserving_state();
	void load_plugins_from_paths(const std::vector<std::string> & paths, const std::string & base_path);
	void display_record_in_view(const conflict_entry_t & entry);
	QString build_mode_prefix() const;
	void start_lua_scan();
	void on_lua_scan_complete(const lua_scan_result_t & result);

	settings_store_t & m_settings;
	plugin_session_t * m_session = nullptr;
	merge_controller_t * m_merge_controller = nullptr;
	view_context_menu_t * m_context_menu = nullptr;

	bool m_conflicts_only = false;
	QLabel * m_lbl_count = nullptr;

	QSplitter * m_main_splitter = nullptr;
	QSplitter * m_content_splitter = nullptr;
	QTabWidget * m_top_tabs = nullptr;
	nav_tree_view_t * m_nav_view = nullptr;
	record_view_t * m_record_view = nullptr;
	messages_view_t * m_messages = nullptr;
	preview_view_t * m_preview = nullptr;
	QTabWidget * m_bottom_tabs = nullptr;

	lua_conflicts_view_t * m_lua_conflicts_view = nullptr;
	lua_scan_worker_t * m_lua_scan_worker = nullptr;

	QLabel * m_status_label = nullptr;

	bool m_filter_active = false;
	nav_tree_model_t::filter_state_t m_last_filter_state;

	bool m_has_filter_active = false;
	nav_tree_model_t::filter_state_t m_last_quick_filter;
	bool m_hide_duplicates = false;
};
