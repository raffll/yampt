#pragma once

#include "menu_action.hpp"
#include <io/file_list.hpp>
#include <string>
#include <vector>

inline constexpr const char * workspace_label = "Workspace";

class session_t;

struct sidebar_render_item_t
{
	std::string path;
	std::string display_text;
	file_type_t type = file_type_t::user_dict;
	bool is_workspace = false;
	bool is_native_yaml = false;
};

struct sidebar_render_node_t
{
	std::string label;
	std::string root_path;
	std::string folder_path;
	std::vector<sidebar_render_item_t> items;
	std::vector<sidebar_render_node_t> children;
};

struct sidebar_render_model_t
{
	std::vector<sidebar_render_node_t> roots;
	std::string active_path;
};

std::string derive_display_name(const file_entry_t & entry, bool is_loaded, bool is_dirty);
std::vector<menu_action_t> derive_context_menu(const file_entry_t & entry, bool is_loaded, bool is_dirty);
std::string derive_output_dir(const file_entry_t & entry, const std::string & default_dir);
sidebar_render_model_t build_render_model(
    const file_list_t & file_list,
    const session_t & session,
    const std::string & active_path);
