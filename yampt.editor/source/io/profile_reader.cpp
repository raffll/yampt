#include "profile_reader.hpp"
#include <utility/string_utils.hpp>
#include <filesystem>
#include <fstream>
#include <string>

static std::string trim_line(const std::string & line)
{
	auto start = line.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return {};

	auto end_pos = line.find_last_not_of(" \t\r\n");
	return line.substr(start, end_pos - start + 1);
}

std::vector<std::string> parse_loadorder_file(const std::string & path)
{
	std::ifstream input(path);
	if (!input.is_open())
		return {};

	std::vector<std::string> plugin_names;
	std::string line_buffer;

	while (std::getline(input, line_buffer))
	{
		auto trimmed = trim_line(line_buffer);
		if (trimmed.empty())
			continue;

		if (trimmed[0] == '#')
			continue;

		if (trimmed[0] == '*')
			trimmed = trimmed.substr(1);

		if (!trimmed.empty())
			plugin_names.push_back(trimmed);
	}

	return plugin_names;
}

std::vector<std::string> parse_modlist_file(const std::string & path)
{
	std::ifstream input(path);
	if (!input.is_open())
		return {};

	std::vector<std::string> enabled_mods;
	std::string line_buffer;

	while (std::getline(input, line_buffer))
	{
		auto trimmed = trim_line(line_buffer);
		if (trimmed.empty())
			continue;

		if (trimmed[0] == '-')
			continue;

		if (trimmed[0] == '+')
			trimmed = trimmed.substr(1);

		if (!trimmed.empty())
			enabled_mods.push_back(trimmed);
	}

	std::reverse(enabled_mods.begin(), enabled_mods.end());
	return enabled_mods;
}

std::string resolve_game_data_path(const std::string & mo2_root_path)
{
	namespace fs = std::filesystem;

	const auto ini_path = fs::path(mo2_root_path) / "ModOrganizer.ini";
	std::ifstream input(ini_path);
	if (!input.is_open())
		return {};

	std::string line_buffer;
	std::string game_path;

	while (std::getline(input, line_buffer))
	{
		auto trimmed = trim_line(line_buffer);
		if (trimmed.rfind("gamePath=", 0) != 0 && trimmed.rfind("gamePath =", 0) != 0)
			continue;

		auto equals_pos = trimmed.find('=');
		if (equals_pos == std::string::npos)
			continue;

		game_path = trim_line(trimmed.substr(equals_pos + 1));

		if (game_path.size() > 12 && game_path.rfind("@ByteArray(", 0) == 0 && game_path.back() == ')')
			game_path = game_path.substr(11, game_path.size() - 12);

		break;
	}

	if (game_path.empty())
		return {};

	game_path = string_utils::normalize_path(game_path);

	if (game_path.size() < 11 || game_path.substr(game_path.size() - 11) != "/Data Files")
		game_path += "/Data Files";

	return game_path;
}

std::string resolve_plugin_in_mods(
    const std::string & plugin_name,
    const std::string & mods_path,
    const std::vector<std::string> & enabled_mods,
    const std::string & game_data_path)
{
	namespace fs = std::filesystem;

	for (const auto & mod_name : enabled_mods)
	{
		auto candidate = fs::path(mods_path) / mod_name / plugin_name;
		if (fs::exists(candidate))
			return candidate.string();
	}

	if (!game_data_path.empty())
	{
		auto candidate = fs::path(game_data_path) / plugin_name;
		if (fs::exists(candidate))
			return candidate.string();
	}

	return {};
}

openmw_cfg_result_t parse_openmw_cfg_file(const std::string & path)
{
	std::ifstream input(path);
	if (!input.is_open())
		return {};

	openmw_cfg_result_t result;
	std::string line_buffer;

	while (std::getline(input, line_buffer))
	{
		auto trimmed = trim_line(line_buffer);

		bool is_data = (trimmed.rfind("data=", 0) == 0 || trimmed.rfind("data =", 0) == 0);
		bool is_content = (trimmed.rfind("content=", 0) == 0 || trimmed.rfind("content =", 0) == 0);

		if (!is_data && !is_content)
			continue;

		auto equals_pos = trimmed.find('=');
		if (equals_pos == std::string::npos)
			continue;

		auto value = trim_line(trimmed.substr(equals_pos + 1));

		if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
			value = value.substr(1, value.size() - 2);

		if (is_data)
			result.data_dirs.push_back(value);
		else
			result.content_names.push_back(value);
	}

	return result;
}
