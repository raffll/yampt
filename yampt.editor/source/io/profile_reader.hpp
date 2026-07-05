#pragma once

#include <string>
#include <vector>

struct openmw_cfg_result_t
{
	std::vector<std::string> data_dirs;
	std::vector<std::string> content_names;
};

std::vector<std::string> parse_loadorder_file(const std::string & path);
std::vector<std::string> parse_modlist_file(const std::string & path);
std::string resolve_game_data_path(const std::string & mo2_root_path);
std::string resolve_plugin_in_mods(
    const std::string & plugin_name,
    const std::string & mods_path,
    const std::vector<std::string> & enabled_mods,
    const std::string & game_data_path);
openmw_cfg_result_t parse_openmw_cfg_file(const std::string & path);
