#include "file_list.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <map>

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

static std::string normalize_path(std::string path)
{
	std::replace(path.begin(), path.end(), '\\', '/');
	return path;
}

const file_entry_t * file_list_t::get(const std::string & path) const
{
	const auto it = entries_.find(normalize_path(path));
	if (it == entries_.end())
		return nullptr;

	return &it->second;
}

file_entry_t * file_list_t::get(const std::string & path)
{
	auto it = entries_.find(normalize_path(path));
	if (it == entries_.end())
		return nullptr;

	return &it->second;
}

bool file_list_t::contains(const std::string & path) const
{
	return entries_.count(normalize_path(path)) > 0;
}

std::vector<const file_entry_t *> file_list_t::all() const
{
	std::vector<const file_entry_t *> result;
	result.reserve(entries_.size());
	for (const auto & [key, entry] : entries_)
		result.push_back(&entry);

	return result;
}

std::vector<const file_entry_t *> file_list_t::workspace_files() const
{
	std::vector<const file_entry_t *> result;
	for (const auto & [key, entry] : entries_)
		result.push_back(&entry);

	return result;
}

file_entry_t & file_list_t::add(const std::string & path)
{
	const auto normalized = normalize_path(path);
	auto it = entries_.find(normalized);
	if (it != entries_.end())
		return it->second;

	file_entry_t entry;
	entry.path = normalized;
	entry.filename = extract_filename(normalized);
	entry.type = classify(normalized);

	auto [inserted_it, success] = entries_.emplace(normalized, std::move(entry));
	return inserted_it->second;
}

void file_list_t::remove(const std::string & path)
{
	entries_.erase(normalize_path(path));
}

void file_list_t::set_loaded(const std::string & path, bool loaded)
{
	auto * entry = get(path);
	if (!entry)
		return;

	entry->dict_loaded = loaded;
	if (!loaded)
		entry->dirty = false;
}

void file_list_t::set_dirty(const std::string & path, bool dirty)
{
	auto * entry = get(path);
	if (!entry)
		return;

	entry->dirty = dirty;
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
		return {
			menu_action_t::make_dict,
			menu_action_t::make_dict_with_base,
			menu_action_t::make_base,
			menu_action_t::convert,
			menu_action_t::create_plugin,
			menu_action_t::delete_file
		};
	}

	if (entry.type == file_type_t::base_dict || entry.type == file_type_t::user_dict)
	{
		if (entry.dict_loaded && entry.dirty)
			return {menu_action_t::save, menu_action_t::delete_file};

		return {menu_action_t::delete_file};
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

void file_list_t::scan_roots(const std::vector<std::string> & root_paths)
{
	clear_workspace();
	roots_ = root_paths;

	for (const auto & root : root_paths)
		scan_single_root(root);
}

const std::vector<std::string> & file_list_t::get_roots() const
{
	return roots_;
}

void file_list_t::scan_single_root(const std::string & root_path)
{
	std::error_code ec;
	if (!std::filesystem::is_directory(root_path, ec))
		return;

	const auto root_fs = std::filesystem::path(root_path);
	const auto normalized_root = normalize_path(root_path);

	for (const auto & entry : std::filesystem::recursive_directory_iterator(root_path, ec))
	{
		if (!entry.is_regular_file(ec))
			continue;

		const auto ext = to_lower(entry.path().extension().string());
		if (ext != ".esm" && ext != ".esp" && ext != ".json" && ext != ".xml"
			&& ext != ".yaml" && ext != ".omwaddon" && ext != ".omwgame")
			continue;

		const auto path = entry.path().string();
		auto & fe = add(path);
		fe.is_workspace = true;
		fe.root_path = normalized_root;

		const auto parent = entry.path().parent_path();
		const auto relative = std::filesystem::relative(parent, root_fs, ec);
		if (!ec && relative != ".")
			fe.workspace_subfolder = relative.string();
		else
			fe.workspace_subfolder = "";

		if (fe.type == file_type_t::plugin)
		{
			const auto size = entry.file_size(ec);
			if (!ec)
				fe.language_tag = detect_language(fe.filename, size);
		}
	}
}

sidebar_render_model_t build_render_model(const file_list_t & file_list, const std::string & active_path)
{
	sidebar_render_model_t model;
	model.active_path = active_path;

	auto sort_items = [](std::vector<sidebar_render_item_t> & items)
	{
		std::sort(items.begin(), items.end(),
			[](const sidebar_render_item_t & a, const sidebar_render_item_t & b)
			{
				if (a.type != b.type)
					return static_cast<int>(a.type) < static_cast<int>(b.type);

				auto fname = [](const std::string & p) {
					auto pos = p.find_last_of("/\\");
					return pos != std::string::npos ? p.substr(pos + 1) : p;
				};
				return fname(a.path) < fname(b.path);
			});
	};

	auto split_path = [](const std::string & path) -> std::vector<std::string>
	{
		std::vector<std::string> parts;
		std::string segment;
		for (auto c : path)
		{
			if (c == '/' || c == '\\')
			{
				if (!segment.empty())
				{
					parts.push_back(segment);
					segment.clear();
				}
				continue;
			}

			segment += c;
		}

		if (!segment.empty())
			parts.push_back(segment);

		return parts;
	};

	struct tree_builder_t
	{
		std::vector<sidebar_render_item_t> items;
		std::map<std::string, tree_builder_t> children;
	};

	std::map<std::string, tree_builder_t> roots_map;

	for (const auto & root : file_list.get_roots())
		roots_map[normalize_path(root)];

	for (const auto * entry : file_list.all())
	{
		if (!entry->is_workspace)
			continue;

		sidebar_render_item_t item;
		item.path = entry->path;
		item.display_text = derive_display_name(*entry);
		item.type = entry->type;
		item.is_workspace = true;

		auto & root_builder = roots_map[entry->root_path];

		if (entry->workspace_subfolder.empty())
		{
			root_builder.items.push_back(std::move(item));
			continue;
		}

		const auto segments = split_path(entry->workspace_subfolder);
		auto * current = &root_builder;
		for (const auto & seg : segments)
			current = &current->children[seg];

		current->items.push_back(std::move(item));
	}

	std::function<sidebar_render_node_t(const std::string &, const std::string &, tree_builder_t &)> build_node;
	build_node = [&](const std::string & label, const std::string & parent_path, tree_builder_t & builder) -> sidebar_render_node_t
	{
		sidebar_render_node_t node;
		node.label = label;
		node.folder_path = parent_path.empty() ? "" : parent_path + "/" + label;

		sort_items(builder.items);
		node.items = std::move(builder.items);

		const auto & current_path = node.folder_path.empty() ? parent_path : node.folder_path;
		for (auto & [child_name, child_builder] : builder.children)
			node.children.push_back(build_node(child_name, current_path, child_builder));

		std::sort(node.children.begin(), node.children.end(),
			[](const sidebar_render_node_t & a, const sidebar_render_node_t & b)
			{
				return a.label < b.label;
			});

		return node;
	};

	for (auto & [root_path, root_builder] : roots_map)
	{
		auto label = extract_filename(root_path);
		if (label == "workspace")
			label = "Workspace";

		auto root_node = build_node(label, root_path, root_builder);
		root_node.root_path = root_path;
		root_node.folder_path = root_path;
		model.roots.push_back(std::move(root_node));
	}

	std::sort(model.roots.begin(), model.roots.end(),
		[](const sidebar_render_node_t & a, const sidebar_render_node_t & b)
		{
			if (a.label == "Workspace")
				return true;

			if (b.label == "Workspace")
				return false;

			return a.label < b.label;
		});

	return model;
}
