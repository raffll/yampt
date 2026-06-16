#include "file_list.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>

static std::string to_lower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return str;
}

static std::string extract_filename(const std::string & path)
{
	const auto pos = path.find_last_of("\\/");
	if (pos == std::string::npos)
		return path;

	return path.substr(pos + 1);
}

static std::string extract_extension(const std::string & path)
{
	const auto & filename = extract_filename(path);
	const auto pos = filename.rfind('.');
	if (pos == std::string::npos)
		return "";

	return to_lower(filename.substr(pos));
}

static std::string parent_directory(const std::string & path)
{
	const auto pos = path.find_last_of("\\/");
	if (pos == std::string::npos)
		return "";

	return path.substr(0, pos);
}

const file_entry_t * file_list_t::get(const std::string & path) const
{
	const auto it = entries_.find(path);
	if (it == entries_.end())
		return nullptr;

	return &it->second;
}

file_entry_t * file_list_t::get(const std::string & path)
{
	auto it = entries_.find(path);
	if (it == entries_.end())
		return nullptr;

	return &it->second;
}

bool file_list_t::contains(const std::string & path) const
{
	return entries_.count(path) > 0;
}

std::vector<const file_entry_t *> file_list_t::all() const
{
	std::vector<const file_entry_t *> result;
	result.reserve(entries_.size());
	for (const auto & [key, entry] : entries_)
		result.push_back(&entry);

	return result;
}

std::vector<const file_entry_t *> file_list_t::loaded_non_workspace() const
{
	std::vector<const file_entry_t *> result;
	for (const auto & [key, entry] : entries_)
	{
		if (entry.loaded && !entry.is_workspace)
			result.push_back(&entry);
	}

	return result;
}

std::vector<const file_entry_t *> file_list_t::workspace_files() const
{
	std::vector<const file_entry_t *> result;
	for (const auto & [key, entry] : entries_)
	{
		if (entry.is_workspace)
			result.push_back(&entry);
	}

	return result;
}

file_entry_t & file_list_t::add(const std::string & path)
{
	auto it = entries_.find(path);
	if (it != entries_.end())
		return it->second;

	file_entry_t entry;
	entry.path = path;
	entry.filename = extract_filename(path);
	entry.type = classify(path);

	auto [inserted_it, success] = entries_.emplace(path, std::move(entry));
	return inserted_it->second;
}

void file_list_t::remove(const std::string & path)
{
	entries_.erase(path);
}

void file_list_t::set_loaded(const std::string & path, bool loaded)
{
	auto * entry = get(path);
	if (!entry)
		return;

	entry->loaded = loaded;
	if (!loaded)
		entry->dirty = false;
}

void file_list_t::set_dirty(const std::string & path, bool dirty)
{
	auto * entry = get(path);
	if (!entry)
		return;

	if (!entry->loaded)
		return;

	entry->dirty = dirty;
}

std::vector<std::string> file_list_t::paths_to_persist() const
{
	std::vector<std::string> result;
	for (const auto & [key, entry] : entries_)
	{
		if (entry.loaded && !entry.is_workspace)
			result.push_back(entry.path);
	}

	return result;
}

file_type_t file_list_t::classify(const std::string & path)
{
	const auto ext = extract_extension(path);
	if (ext == ".esm" || ext == ".esp" || ext == ".omwgame" || ext == ".omwaddon")
		return file_type_t::plugin;

	if (ext == ".yaml")
		return file_type_t::lua_l10n;

	if (ext == ".json" || ext == ".xml")
	{
		const auto filename = to_lower(extract_filename(path));
		if (filename.find("_base_") != std::string::npos)
			return file_type_t::base_dict;

		return file_type_t::user_dict;
	}

	return file_type_t::user_dict;
}

std::string file_list_t::detect_language(const std::string & filename, std::uintmax_t file_size)
{
	const auto lower = to_lower(filename);

	if (lower != "morrowind.esm" && lower != "tribunal.esm" && lower != "bloodmoon.esm")
		return "";

	if (file_size == 79837557 || file_size == 9631798 || file_size == 4565686) return "EN";
	if (file_size == 80640776 || file_size == 9797295 || file_size == 6069165) return "DE";
	if (file_size == 80105097 || file_size == 9658076 || file_size == 4626565) return "PL";
	if (file_size == 80681814 || file_size == 10015689 || file_size == 4697358) return "FR";
	if (file_size == 79857000 || file_size == 9702000 || file_size == 4625000) return "RU";

	return "";
}

file_type_t classify(const std::string & path)
{
	const auto ext = extract_extension(path);
	if (ext == ".esm" || ext == ".esp")
		return file_type_t::plugin;

	if (ext == ".yaml")
		return file_type_t::lua_l10n;

	if (ext == ".json" || ext == ".xml")
	{
		const auto filename = to_lower(extract_filename(path));
		if (filename.find("_base_") != std::string::npos)
			return file_type_t::base_dict;

		return file_type_t::user_dict;
	}

	return file_type_t::user_dict;
}

std::string detect_language(const std::string & filename, std::uintmax_t file_size)
{
	const auto lower = to_lower(filename);

	if (lower == "morrowind.esm")
	{
		if (file_size == 79837557) return "EN";
		if (file_size == 80640776) return "DE";
		if (file_size == 80105097) return "PL";
		if (file_size == 80681814) return "FR";
		if (file_size == 79857000) return "RU";
		return "";
	}

	if (lower == "tribunal.esm")
	{
		if (file_size == 9631798) return "EN";
		if (file_size == 9797295) return "DE";
		if (file_size == 9658076) return "PL";
		if (file_size == 10015689) return "FR";
		if (file_size == 9702000) return "RU";
		return "";
	}

	if (lower == "bloodmoon.esm")
	{
		if (file_size == 4565686) return "EN";
		if (file_size == 6069165) return "DE";
		if (file_size == 4626565) return "PL";
		if (file_size == 4697358) return "FR";
		if (file_size == 4625000) return "RU";
		return "";
	}

	return "";
}

std::string derive_display_name(const file_entry_t & entry)
{
	std::string result;

	if (entry.dirty)
		result += "* ";

	switch (entry.type)
	{
	case file_type_t::plugin:
		result += "[ESP]";
		break;
	case file_type_t::base_dict:
		result += "[BASE]";
		break;
	case file_type_t::user_dict:
		result += "[USER]";
		break;
	case file_type_t::lua_l10n:
		result += "[LUA]";
		break;
	}

	if (entry.type == file_type_t::plugin && !entry.language_tag.empty())
		result += " [" + entry.language_tag + "]";

	result += " " + entry.filename;

	return result;
}

std::vector<menu_action_t> derive_context_menu(const file_entry_t & entry)
{
	if (entry.type == file_type_t::lua_l10n)
		return {menu_action_t::delete_file};

	if (entry.type == file_type_t::plugin)
	{
		if (entry.loaded && !entry.is_workspace)
		{
			return {
				menu_action_t::make_dict,
				menu_action_t::make_dict_with_base,
				menu_action_t::make_base,
				menu_action_t::convert,
				menu_action_t::create_plugin,
				menu_action_t::unload
			};
		}

		if (!entry.loaded && entry.is_workspace)
		{
			return {
				menu_action_t::make_dict,
				menu_action_t::make_dict_with_base,
				menu_action_t::make_base,
				menu_action_t::convert,
				menu_action_t::create_plugin,
				menu_action_t::delete_file
			};
		}

		return {};
	}

	if (entry.type == file_type_t::base_dict || entry.type == file_type_t::user_dict)
	{
		if (entry.loaded && !entry.is_workspace)
			return {menu_action_t::save, menu_action_t::save_as, menu_action_t::unload};

		if (entry.loaded && entry.is_workspace && entry.dirty)
			return {menu_action_t::save, menu_action_t::delete_file};

		if (entry.loaded && entry.is_workspace && !entry.dirty)
			return {menu_action_t::delete_file};

		if (!entry.loaded && entry.is_workspace)
			return {menu_action_t::delete_file};

		return {};
	}

	return {};
}

std::string derive_output_dir(const file_entry_t & entry, const std::string & default_dir)
{
	if (entry.is_workspace)
		return parent_directory(entry.path);

	return default_dir;
}

void file_list_t::clear_workspace()
{
	for (auto it = entries_.begin(); it != entries_.end(); )
	{
		if (it->second.is_workspace)
			it = entries_.erase(it);
		else
			++it;
	}
}

void file_list_t::scan_workspace(const std::string & workspace_dir)
{
	clear_workspace();

	std::error_code ec;
	if (!std::filesystem::is_directory(workspace_dir, ec))
		return;

	for (const auto & entry : std::filesystem::directory_iterator(workspace_dir, ec))
	{
		if (entry.is_regular_file(ec))
		{
			const auto path = entry.path().string();
			auto & fe = add(path);
			fe.is_workspace = true;
			fe.workspace_subfolder = "";

			if (fe.type == file_type_t::plugin)
			{
				auto size = entry.file_size(ec);
				if (!ec)
					fe.language_tag = detect_language(fe.filename, size);
			}
			continue;
		}

		if (!entry.is_directory(ec))
			continue;

		const auto subfolder = entry.path().filename().string();
		for (const auto & sub_entry : std::filesystem::directory_iterator(entry.path(), ec))
		{
			if (!sub_entry.is_regular_file(ec))
				continue;

			const auto path = sub_entry.path().string();
			auto & fe = add(path);
			fe.is_workspace = true;
			fe.workspace_subfolder = subfolder;

			if (fe.type == file_type_t::plugin)
			{
				auto size = sub_entry.file_size(ec);
				if (!ec)
					fe.language_tag = detect_language(fe.filename, size);
			}
		}
	}
}

sidebar_render_model_t build_render_model(const file_list_t & file_list, const std::string & active_path)
{
	sidebar_render_model_t model;
	model.active_path = active_path;
	model.loaded_root.label = "Loaded";
	model.workspace_root.label = "Workspace";

	std::unordered_map<std::string, std::vector<sidebar_render_item_t>> subfolder_items;

	for (const auto * entry : file_list.all())
	{
		if (entry->loaded && !entry->is_workspace)
		{
			sidebar_render_item_t item;
			item.path = entry->path;
			item.display_text = derive_display_name(*entry);
			item.is_workspace = false;
			model.loaded_root.items.push_back(std::move(item));
			continue;
		}

		if (!entry->is_workspace)
			continue;

		sidebar_render_item_t item;
		item.path = entry->path;
		item.display_text = derive_display_name(*entry);
		item.is_workspace = true;

		if (entry->workspace_subfolder.empty())
		{
			model.workspace_root.items.push_back(std::move(item));
			continue;
		}

		subfolder_items[entry->workspace_subfolder].push_back(std::move(item));
	}

	auto sort_items = [](std::vector<sidebar_render_item_t> & items)
	{
		std::sort(items.begin(), items.end(),
			[](const sidebar_render_item_t & a, const sidebar_render_item_t & b)
			{
				return a.display_text < b.display_text;
			});
	};

	sort_items(model.loaded_root.items);
	sort_items(model.workspace_root.items);

	for (auto & [subfolder, items] : subfolder_items)
	{
		sidebar_render_node_t child;
		child.label = subfolder + "/";
		child.items = std::move(items);
		sort_items(child.items);
		model.workspace_root.children.push_back(std::move(child));
	}

	std::sort(model.workspace_root.children.begin(), model.workspace_root.children.end(),
		[](const sidebar_render_node_t & a, const sidebar_render_node_t & b)
		{
			return a.label < b.label;
		});

	return model;
}
