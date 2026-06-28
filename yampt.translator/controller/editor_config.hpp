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
	std::vector<std::string> workspace_roots;
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
	std::string active_dict_path;
	std::vector<std::string> last_merge_order;
	std::string deepl_api_key;
	int translation_source_index = 0;
	int translation_language_index = 0;
	int window_x = -1;
	int window_y = -1;
	int window_w = 1280;
	int window_h = 720;
	bool window_maximized = false;
};
