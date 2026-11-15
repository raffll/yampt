#pragma once

#include <set>
#include <string>
#include <vector>

namespace save_routing {

enum class save_path_t { created_plugin, loaded_plugin };

struct master_selection_input_t
{
	const std::vector<std::string> & plugin_filenames;
	const std::set<std::string> & copy_source_filenames;
	int active_index = -1;
};

save_path_t decide_save_path(const std::string & filename, const std::set<std::string> & created_plugins);

std::vector<int> select_copy_source_indices(const master_selection_input_t & input);

} // namespace save_routing
