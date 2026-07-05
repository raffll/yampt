#pragma once

#include <functional>
#include <string>
#include <vector>

struct openmw_cfg_t
{
	std::vector<std::string> data_dirs;
	std::vector<std::string> content_names;
};

struct mo2_profile_t
{
	std::vector<std::string> plugin_names;
	std::vector<std::string> enabled_mods;
};

enum class load_source_kind_t
{
	folder,
	mo2_profile,
	openmw_cfg
};

using file_exists_fn_t = std::function<bool(const std::string &)>;

class profile_reader_t
{
public:
	static openmw_cfg_t parse_openmw_cfg_text(const std::string & text);

	static mo2_profile_t parse_mo2_profile_text(const std::string & loadorder_text, const std::string & modlist_text);

	static std::vector<std::string> resolve_openmw_content(
	    const std::vector<std::string> & content_names,
	    const std::vector<std::string> & data_dirs,
	    const file_exists_fn_t & file_exists);

	static std::string resolve_single_openmw_content(
	    const std::string & content_name,
	    const std::vector<std::string> & data_dirs,
	    const file_exists_fn_t & file_exists);

	static std::string resolve_mo2_plugin(
	    const std::string & plugin_name,
	    const std::string & overwrite_path,
	    const std::vector<std::string> & enabled_mods,
	    const std::string & mods_path,
	    const std::string & game_data_path,
	    const file_exists_fn_t & file_exists);

	static std::string resolve_merge_output_path(load_source_kind_t source, const std::string & base_path);
};
