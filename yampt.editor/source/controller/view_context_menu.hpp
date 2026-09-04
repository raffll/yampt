#pragma once

#include "../model/field_binary_resolver.hpp"
#include "../model/nav_tree_model.hpp"
#include "../model/view_tree_model.hpp"
#include <scanner/merge_patch_store.hpp>
#include <functional>
#include <QModelIndex>
#include <QPoint>

class merge_controller_t;
class plugin_session_t;
class record_view_t;
class nav_tree_view_t;
class settings_store_t;
class QMenu;

class view_context_menu_t
{
public:
	using settings_changed_fn = std::function<void()>;
	using unsaved_changed_fn = std::function<void(bool)>;

	view_context_menu_t(
	    plugin_session_t & session,
	    record_view_t & record_view,
	    nav_tree_view_t & nav_view,
	    merge_controller_t & merge_controller,
	    settings_store_t & settings,
	    settings_changed_fn on_settings_changed,
	    unsaved_changed_fn on_unsaved_changed);

	void show_view_menu(const QPoint & global_pos, const QModelIndex & index);
	void show_nav_menu(const QPoint & global_pos, const nav_tree_model_t::node_info_t & info);

private:
	enum class row_kind_t
	{
		sub_record,
		schema_record,
		group,
		field_of_schema,
		field_of_group,
		other
	};

	struct view_menu_context_t
	{
		const QModelIndex & index;
		const view_tree_model_t::view_node_t & row;
		const std::string & rec_type;
		const std::string & record_id;
		int plugin_idx;
		int col;
		int bin_idx;
		int parent_row_idx;
		row_kind_t kind;
	};

	void build_source_file_menu(QMenu & menu, const nav_tree_model_t::node_info_t & info);
	void confirm_remove_record_from_plugin(const nav_tree_model_t::node_info_t & info);
	void build_copy_to_merge_menu(QMenu & menu, const view_menu_context_t & context);
	void build_source_copy_menu(QMenu & menu, const view_menu_context_t & context);
	void build_merge_remove_menu(QMenu & menu, const view_menu_context_t & context);
	void build_lock_menu(QMenu & menu, const view_menu_context_t & context);
	merge_lock_t build_lock_for(const view_menu_context_t & context) const;
	void build_sub_record_ignore_menu(QMenu & menu, const view_menu_context_t & context);
	void toggle_ignore_rule(const std::string & rule, bool remove_rule);
	void add_copy_bit_action(QMenu & menu, const view_menu_context_t & context);
	field_binary_resolver::resolved_field_t resolve_schema_field(const view_menu_context_t & context) const;
	field_binary_resolver::resolved_bit_t resolve_schema_bit(const view_menu_context_t & context) const;

	plugin_session_t & m_session;
	record_view_t & m_record_view;
	nav_tree_view_t & m_nav_view;
	merge_controller_t & m_merge;
	settings_store_t & m_settings;
	settings_changed_fn m_on_settings_changed;
	unsaved_changed_fn m_on_unsaved_changed;
};
