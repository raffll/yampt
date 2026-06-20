#include "sidebar_model.hpp"
#include "display_name.hpp"
#include "session.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>

static std::string extract_filename(const std::string & path)
{
	const auto pos = path.find_last_of("\\/");
	if (pos == std::string::npos)
		return path;

	return path.substr(pos + 1);
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

std::string derive_display_name(const file_entry_t & entry, bool is_loaded, bool is_dirty)
{
	display_name_t name(entry.filename);
	name.set_dirty(is_dirty);
	name.set_file_type(entry.type);

	if ((entry.type == file_type_t::base_dict || entry.type == file_type_t::user_dict) && !is_loaded)
		name.set_unloaded(true);

	if (entry.type == file_type_t::base_dict)
		name.set_kind(dict_kind_t::base);

	if (entry.type == file_type_t::plugin && !entry.language_tag.empty())
		name.set_language(entry.language_tag);

	if (entry.type == file_type_t::yaml_l10n)
	{
		std::error_code ec;
		if (std::filesystem::exists(entry.path + ".tmp", ec))
			name.set_wip(true);
	}

	return name.to_string();
}

std::vector<menu_action_t> derive_context_menu(const file_entry_t & entry, bool is_loaded, bool is_dirty)
{
	if (entry.type == file_type_t::yaml_l10n)
		return { menu_action_t::delete_file };

	if (entry.type == file_type_t::plugin)
	{
		return { menu_action_t::make_dict, menu_action_t::make_dict_with_base, menu_action_t::make_base,
			     menu_action_t::convert,   menu_action_t::create_plugin,       menu_action_t::delete_file };
	}

	if (entry.type == file_type_t::base_dict || entry.type == file_type_t::user_dict)
	{
		if (is_loaded && is_dirty)
			return { menu_action_t::save, menu_action_t::delete_file };

		return { menu_action_t::delete_file };
	}

	return {};
}

std::string derive_output_dir(const file_entry_t & entry, const std::string & default_dir)
{
	if (entry.is_workspace)
		return parent_directory(entry.path);

	return default_dir;
}

sidebar_render_model_t build_render_model(
    const file_list_t & file_list,
    const session_t & session,
    const std::string & active_path)
{
	sidebar_render_model_t model;
	model.active_path = active_path;

	auto sort_items = [](std::vector<sidebar_render_item_t> & items)
	{
		std::sort(
		    items.begin(),
		    items.end(),
		    [](const sidebar_render_item_t & a, const sidebar_render_item_t & b)
		{
			if (a.type != b.type)
				return static_cast<int>(a.type) < static_cast<int>(b.type);

			auto fname = [](const std::string & p)
			{
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

		const auto * doc = session.find(entry->path);
		const bool is_loaded = (doc != nullptr);
		const bool is_dirty = doc && doc->is_dirty();

		sidebar_render_item_t item;
		item.path = entry->path;
		item.display_text = derive_display_name(*entry, is_loaded, is_dirty);
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
	build_node = [&](const std::string & label,
	                 const std::string & parent_path,
	                 tree_builder_t & builder) -> sidebar_render_node_t
	{
		sidebar_render_node_t node;
		node.label = label;
		node.folder_path = parent_path.empty() ? "" : parent_path + "/" + label;

		sort_items(builder.items);
		node.items = std::move(builder.items);

		const auto & current_path = node.folder_path.empty() ? parent_path : node.folder_path;
		for (auto & [child_name, child_builder] : builder.children)
			node.children.push_back(build_node(child_name, current_path, child_builder));

		std::sort(
		    node.children.begin(),
		    node.children.end(),
		    [](const sidebar_render_node_t & a, const sidebar_render_node_t & b) { return a.label < b.label; });

		return node;
	};

	for (auto & [root_path, root_builder] : roots_map)
	{
		auto label = extract_filename(root_path);
		if (label == "workspace")
			label = workspace_label;

		sidebar_render_node_t root_node;
		root_node.label = label;
		root_node.root_path = root_path;
		root_node.folder_path = root_path;

		sort_items(root_builder.items);
		root_node.items = std::move(root_builder.items);

		for (auto & [child_name, child_builder] : root_builder.children)
			root_node.children.push_back(build_node(child_name, root_path, child_builder));

		std::sort(
		    root_node.children.begin(),
		    root_node.children.end(),
		    [](const sidebar_render_node_t & a, const sidebar_render_node_t & b) { return a.label < b.label; });

		model.roots.push_back(std::move(root_node));
	}

	std::sort(
	    model.roots.begin(),
	    model.roots.end(),
	    [](const sidebar_render_node_t & a, const sidebar_render_node_t & b)
	{
		if (a.label == workspace_label)
			return true;

		if (b.label == workspace_label)
			return false;

		return a.label < b.label;
	});

	return model;
}
