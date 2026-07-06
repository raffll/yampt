#pragma once

#include "../model/nav_tree_model.hpp"
#include <QModelIndex>
#include <QPoint>

class merge_controller_t;
class plugin_session_t;
class record_view_t;
class nav_tree_view_t;
class settings_store_t;

class view_context_menu_t
{
public:
	view_context_menu_t(
	    plugin_session_t & session,
	    record_view_t & record_view,
	    nav_tree_view_t & nav_view,
	    merge_controller_t & merge_controller);

	void show_view_menu(const QPoint & global_pos, const QModelIndex & index);
	void show_nav_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info);

private:
	plugin_session_t & m_session;
	record_view_t & m_record_view;
	nav_tree_view_t & m_nav_view;
	merge_controller_t & m_merge;
};
