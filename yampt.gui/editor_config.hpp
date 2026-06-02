#pragma once

#include <array>
#include <string>
#include <vector>

class editor_config_t
{
public:
	void load(const std::string & path);
	void save(const std::string & path) const;

	float split_ratio = 0.5f;
	std::array<float, 4> column_widths = { 150.f, 300.f, 300.f, 80.f };
	std::vector<std::string> base_dict_paths;
	std::string spell_check_aff;
	std::string spell_check_dic;
	std::string last_user_dict_path;
	std::string last_source_dict_path;
};
