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
	std::vector<std::string> user_dict_paths;
	std::vector<std::string> base_dict_paths;
	std::string spell_check_aff;
	std::string spell_check_dic;
	int spell_lang_index = -1;

	float sidebar_width = 250.0f;
	float bottom_height = 200.0f;
	float info_height = 150.0f;
	bool sidebar_visible = true;
	bool bottom_visible = true;
	int encoding_index = 2;
	int active_dict_index = -1;
	int window_x = -1;
	int window_y = -1;
	int window_w = 1280;
	int window_h = 720;
};
