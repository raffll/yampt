#include "folder_plugin_order.hpp"
#include "../utility/string_utils.hpp"

namespace folder_plugin_order {

namespace {

bool is_master_path(const std::string & plugin_path)
{
	return string_utils::to_lower(plugin_path).ends_with(".esm");
}

void append_with_real_file(const std::string & plugin_path, const order_input_t & input, std::vector<std::string> & ordered)
{
	ordered.push_back(plugin_path);

	if (!profile_reader_t::is_openmw_stub_name(plugin_path))
		return;

	const auto real_path = profile_reader_t::real_openmw_name_from_stub(plugin_path);
	if (input.file_exists(real_path))
		ordered.push_back(real_path);
}

} // namespace

std::vector<std::string> order_folder_plugins(const order_input_t & input)
{
	std::vector<std::string> ordered;
	ordered.reserve(input.plugin_paths.size());

	for (const auto & plugin_path : input.plugin_paths)
	{
		if (is_master_path(plugin_path))
			ordered.push_back(plugin_path);
	}

	for (const auto & plugin_path : input.plugin_paths)
	{
		if (!is_master_path(plugin_path))
			append_with_real_file(plugin_path, input, ordered);
	}

	return ordered;
}

} // namespace folder_plugin_order
