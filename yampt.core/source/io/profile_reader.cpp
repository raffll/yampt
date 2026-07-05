#include "profile_reader.hpp"
#include <algorithm>
#include <sstream>

static bool line_starts_with(const std::string & line, const char * prefix)
{
	return line.rfind(prefix, 0) == 0;
}

openmw_cfg_t profile_reader_t::parse_openmw_cfg_text(const std::string & text)
{
	openmw_cfg_t result;
	std::istringstream stream(text);
	std::string line;

	while (std::getline(stream, line))
	{
		while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
			line.pop_back();

		while (!line.empty() && line.front() == ' ')
			line.erase(line.begin());

		if (line_starts_with(line, "data=") || line_starts_with(line, "data ="))
		{
			auto value = line.substr(line.find('=') + 1);
			while (!value.empty() && value.front() == ' ')
				value.erase(value.begin());

			if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
				value = value.substr(1, value.size() - 2);

			if (!value.empty())
				result.data_dirs.push_back(value);
		}
		else if (line_starts_with(line, "content=") || line_starts_with(line, "content ="))
		{
			auto value = line.substr(line.find('=') + 1);
			while (!value.empty() && value.front() == ' ')
				value.erase(value.begin());

			if (!value.empty())
				result.content_names.push_back(value);
		}
	}

	return result;
}

mo2_profile_t profile_reader_t::parse_mo2_profile_text(
    const std::string & loadorder_text,
    const std::string & modlist_text)
{
	mo2_profile_t result;

	std::istringstream loadorder_stream(loadorder_text);
	std::string line;
	while (std::getline(loadorder_stream, line))
	{
		while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
			line.pop_back();

		while (!line.empty() && line.front() == ' ')
			line.erase(line.begin());

		if (line.empty() || line.front() == '#')
			continue;

		if (line.front() == '*')
			line = line.substr(1);

		if (!line.empty())
			result.plugin_names.push_back(line);
	}

	std::istringstream modlist_stream(modlist_text);
	while (std::getline(modlist_stream, line))
	{
		while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
			line.pop_back();

		while (!line.empty() && line.front() == ' ')
			line.erase(line.begin());

		if (line_starts_with(line, "+"))
			result.enabled_mods.push_back(line.substr(1));
	}

	std::reverse(result.enabled_mods.begin(), result.enabled_mods.end());
	return result;
}

std::vector<std::string> profile_reader_t::resolve_openmw_content(
    const std::vector<std::string> & content_names,
    const std::vector<std::string> & data_dirs,
    const file_exists_fn_t & file_exists)
{
	std::vector<std::string> paths;
	for (const auto & name : content_names)
	{
		const auto resolved = resolve_single_openmw_content(name, data_dirs, file_exists);
		if (!resolved.empty())
			paths.push_back(resolved);
	}

	return paths;
}

std::string profile_reader_t::resolve_single_openmw_content(
    const std::string & content_name,
    const std::vector<std::string> & data_dirs,
    const file_exists_fn_t & file_exists)
{
	for (auto it_dir = data_dirs.rbegin(); it_dir != data_dirs.rend(); ++it_dir)
	{
		const auto candidate = *it_dir + "/" + content_name;
		if (file_exists(candidate))
			return candidate;
	}

	return {};
}

std::string profile_reader_t::resolve_mo2_plugin(
    const std::string & plugin_name,
    const std::string & overwrite_path,
    const std::vector<std::string> & enabled_mods,
    const std::string & mods_path,
    const std::string & game_data_path,
    const file_exists_fn_t & file_exists)
{
	if (!overwrite_path.empty())
	{
		const auto candidate = overwrite_path + "/" + plugin_name;
		if (file_exists(candidate))
			return candidate;
	}

	for (const auto & mod_name : enabled_mods)
	{
		const auto candidate = mods_path + "/" + mod_name + "/" + plugin_name;
		if (file_exists(candidate))
			return candidate;
	}

	if (!game_data_path.empty())
	{
		const auto candidate = game_data_path + "/" + plugin_name;
		if (file_exists(candidate))
			return candidate;
	}

	return {};
}

std::string profile_reader_t::resolve_merge_output_path(load_source_kind_t source, const std::string & base_path)
{
	if (base_path.empty())
		return {};

	static const std::string merge_filename = "Merged Patch.esp";

	switch (source)
	{
	case load_source_kind_t::mo2_profile:
	{
		auto path = base_path + "/../../overwrite/" + merge_filename;
		return path;
	}
	case load_source_kind_t::openmw_cfg:
		return base_path + "/data/" + merge_filename;
	case load_source_kind_t::folder:
		return base_path + "/" + merge_filename;
	}

	return base_path + "/" + merge_filename;
}
