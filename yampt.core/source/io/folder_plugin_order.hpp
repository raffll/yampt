#pragma once

#include "profile_reader.hpp"
#include <string>
#include <vector>

namespace folder_plugin_order {

struct order_input_t
{
	const std::vector<std::string> & plugin_paths;
	file_exists_fn_t file_exists;
};

std::vector<std::string> order_folder_plugins(const order_input_t & input);

} // namespace folder_plugin_order
