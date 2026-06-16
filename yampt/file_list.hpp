#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class file_type_t
{
	plugin,
	base_dict,
	user_dict,
	lua_l10n
};

struct file_entry_t
{
	std::string path;
	std::string filename;
	file_type_t type;
	std::string language_tag;
	bool loaded = false;
	bool dirty = false;
	bool is_workspace = false;
	std::string workspace_subfolder;
};

enum class menu_action_t
{
	make_dict,
	make_dict_with_base,
	make_base,
	convert,
	create_plugin,
	save,
	save_as,
	unload,
	delete_file
};

struct sidebar_render_item_t
{
	std::string path;
	std::string display_text;
	bool is_workspace = false;
};

struct sidebar_render_node_t
{
	std::string label;
	std::vector<sidebar_render_item_t> items;
	std::vector<sidebar_render_node_t> children;
};

struct sidebar_render_model_t
{
	sidebar_render_node_t loaded_root;
	sidebar_render_node_t workspace_root;
	std::string active_path;
};

class file_list_t
{
public:
	const file_entry_t * get(const std::string & path) const;
	file_entry_t * get(const std::string & path);
	bool contains(const std::string & path) const;
	std::vector<const file_entry_t *> all() const;
	std::vector<const file_entry_t *> loaded_non_workspace() const;
	std::vector<const file_entry_t *> workspace_files() const;

	file_entry_t & add(const std::string & path);
	void remove(const std::string & path);
	void set_loaded(const std::string & path, bool loaded);
	void set_dirty(const std::string & path, bool dirty);

	void scan_workspace(const std::string & workspace_dir);
	void clear_workspace();
	std::vector<std::string> paths_to_persist() const;

	static file_type_t classify(const std::string & path);
	static std::string detect_language(const std::string & filename, std::uintmax_t file_size);

private:
	std::unordered_map<std::string, file_entry_t> entries_;
};

file_type_t classify(const std::string & path);
std::string detect_language(const std::string & filename, std::uintmax_t file_size);
std::string derive_display_name(const file_entry_t & entry);
std::vector<menu_action_t> derive_context_menu(const file_entry_t & entry);
std::string derive_output_dir(const file_entry_t & entry, const std::string & default_dir);
sidebar_render_model_t build_render_model(const file_list_t & file_list, const std::string & active_path);
