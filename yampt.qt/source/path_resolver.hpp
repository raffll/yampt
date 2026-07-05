#pragma once

#include <string>
#include <vector>

class path_resolver_t
{
public:
	struct search_config_t
	{
		std::vector<std::string> workspace_roots;
		std::string mo2_profile_dir;
		std::string openmw_data_dir;
		std::string last_directory;
	};

	explicit path_resolver_t(const search_config_t & config);

	std::string resolve_workspace_path(const std::string & relative_path) const;
	std::string resolve_mo2_mods_dir() const;
	std::string resolve_mo2_overwrite_dir() const;
	std::string resolve_openmw_data_dir() const;
	std::string resolve_game_data_path() const;
	const std::vector<std::string> & workspace_roots() const;

private:
	search_config_t m_config;
};
