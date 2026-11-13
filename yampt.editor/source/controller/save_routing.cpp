#include "save_routing.hpp"
#include "../session/merged_patch_name.hpp"
#include <utility/string_utils.hpp>

namespace save_routing {

save_path_t decide_save_path(const std::string & filename, const std::set<std::string> & created_plugins)
{
	if (string_utils::case_insensitive_equal(filename, merged_patch::filename))
		return save_path_t::created_plugin;

	for (const auto & created_name : created_plugins)
	{
		if (string_utils::case_insensitive_equal(filename, created_name))
			return save_path_t::created_plugin;
	}

	return save_path_t::loaded_plugin;
}

std::vector<int> select_copy_source_indices(const master_selection_input_t & input)
{
	std::set<std::string> source_names_lower;
	for (const auto & source_name : input.copy_source_filenames)
		source_names_lower.insert(string_utils::to_lower(source_name));

	std::set<std::string> already_selected;
	std::vector<int> selected;
	for (int index = 0; index < static_cast<int>(input.plugin_filenames.size()); ++index)
	{
		if (index == input.active_index)
			continue;

		const auto name_lower = string_utils::to_lower(input.plugin_filenames[index]);
		if (source_names_lower.count(name_lower) == 0)
			continue;

		if (!already_selected.insert(name_lower).second)
			continue;

		selected.push_back(index);
	}

	return selected;
}

} // namespace save_routing
