#include "editor_config.hpp"
#include <fstream>

static std::string trim(const std::string & input)
{
	const auto start = input.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return {};

	const auto finish = input.find_last_not_of(" \t\r\n");
	return input.substr(start, finish - start + 1);
}

static bool starts_with(const std::string & line_text, const std::string & prefix)
{
	if (line_text.size() < prefix.size())
		return false;

	return line_text.compare(0, prefix.size(), prefix) == 0;
}

static int parse_int_safe(const std::string & value_text, int fallback = 0)
{
	try
	{
		return std::stoi(value_text);
	}
	catch (...)
	{
		return fallback;
	}
}

static float parse_float_safe(const std::string & value_text, float fallback = 0.0f)
{
	try
	{
		return std::stof(value_text);
	}
	catch (...)
	{
		return fallback;
	}
}

void editor_config_t::load(const std::string & path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return;

	std::string section;
	std::string line_text;

	while (std::getline(file, line_text))
	{
		line_text = trim(line_text);
		if (line_text.empty())
			continue;

		if (line_text.front() == '[' && line_text.back() == ']')
		{
			section = line_text.substr(1, line_text.size() - 2);
			continue;
		}

		const auto eq_pos = line_text.find('=');
		if (eq_pos == std::string::npos)
			continue;

		const auto key_text = trim(line_text.substr(0, eq_pos));
		const auto value_text = trim(line_text.substr(eq_pos + 1));

		if (section == "WorkspaceRoots")
			parse_workspace_roots(key_text, value_text);
		else if (section == "Editor")
			parse_editor_section(key_text, value_text);
		else if (section == "SpellCheck")
			parse_spell_check(key_text, value_text);
		else if (section == "MergeOrder")
			parse_merge_order(key_text, value_text);
	}
}

void editor_config_t::parse_workspace_roots(const std::string & key_text, const std::string & value_text)
{
	if (key_text == "Count")
	{
		const auto count = parse_int_safe(value_text);
		workspace_roots.clear();
		workspace_roots.reserve(count);
		return;
	}

	if (starts_with(key_text, "Path"))
		workspace_roots.push_back(value_text);
}

void editor_config_t::parse_editor_section(const std::string & key_text, const std::string & value_text)
{
	if (key_text == "ActiveDictIndex")
	{
		active_dict_index = parse_int_safe(value_text, -1);
		return;
	}

	if (key_text == "ActiveDictPath")
	{
		active_dict_path = value_text;
		return;
	}

	if (key_text == "DeepLApiKey")
	{
		deepl_api_key = value_text;
		return;
	}

	if (key_text == "TranslationSourceIndex")
	{
		translation_source_index = parse_int_safe(value_text);
		return;
	}

	if (key_text == "TranslationLanguageIndex")
	{
		translation_language_index = parse_int_safe(value_text);
		return;
	}

	parse_editor_window(key_text, value_text);
}

void editor_config_t::parse_editor_window(const std::string & key_text, const std::string & value_text)
{
	if (key_text == "SplitRatio")
		split_ratio = parse_float_safe(value_text, 0.5f);
	else if (key_text == "SidebarWidth")
		sidebar_width = parse_float_safe(value_text, 250.0f);
	else if (key_text == "BottomHeight")
		bottom_height = parse_float_safe(value_text, 200.0f);
	else if (key_text == "InfoHeight")
		info_height = parse_float_safe(value_text, 150.0f);
	else if (key_text == "SidebarVisible")
		sidebar_visible = (value_text != "0");
	else if (key_text == "BottomVisible")
		bottom_visible = (value_text != "0");
	else if (key_text == "EncodingIndex")
	{
		const auto index = parse_int_safe(value_text);
		if (index >= 0 && index <= 2)
			encoding_index = index;
	}
	else if (key_text == "WindowX")
		window_x = parse_int_safe(value_text, -1);
	else if (key_text == "WindowY")
		window_y = parse_int_safe(value_text, -1);
	else if (key_text == "WindowW")
		window_w = parse_int_safe(value_text, 1280);
	else if (key_text == "WindowH")
		window_h = parse_int_safe(value_text, 720);
	else if (key_text == "WindowMaximized")
		window_maximized = (value_text != "0");
	else if (starts_with(key_text, "Column"))
	{
		const auto col_idx = parse_int_safe(key_text.substr(6), -1);
		if (col_idx >= 0 && col_idx < static_cast<int>(column_widths.size()))
			column_widths[col_idx] = parse_float_safe(value_text);
	}
}

void editor_config_t::parse_spell_check(const std::string & key_text, const std::string & value_text)
{
	if (key_text == "AffPath")
	{
		spell_check_aff = value_text;
		return;
	}

	if (key_text == "DicPath")
	{
		spell_check_dic = value_text;
		return;
	}

	if (key_text == "LangIndex")
		spell_lang_index = parse_int_safe(value_text, -1);
}

void editor_config_t::parse_merge_order(const std::string & key_text, const std::string & value_text)
{
	if (key_text == "Count")
	{
		const auto count = parse_int_safe(value_text);
		last_merge_order.clear();
		last_merge_order.reserve(count);
		return;
	}

	if (starts_with(key_text, "Path"))
		last_merge_order.push_back(value_text);
}

void editor_config_t::save(const std::string & path) const
{
	std::ofstream file(path);
	if (!file.is_open())
		return;

	file << "[WorkspaceRoots]\n";
	file << "Count=" << workspace_roots.size() << "\n";
	for (size_t i = 0; i < workspace_roots.size(); ++i)
		file << "Path" << i << "=" << workspace_roots[i] << "\n";

	file << "\n[Editor]\n";
	file << "ActiveDictIndex=" << active_dict_index << "\n";
	file << "ActiveDictPath=" << active_dict_path << "\n";
	if (!deepl_api_key.empty())
		file << "DeepLApiKey=" << deepl_api_key << "\n";
	file << "TranslationSourceIndex=" << translation_source_index << "\n";
	file << "TranslationLanguageIndex=" << translation_language_index << "\n";
	file << "SplitRatio=" << split_ratio << "\n";
	file << "SidebarWidth=" << sidebar_width << "\n";
	file << "BottomHeight=" << bottom_height << "\n";
	file << "InfoHeight=" << info_height << "\n";
	file << "SidebarVisible=" << (sidebar_visible ? "1" : "0") << "\n";
	file << "BottomVisible=" << (bottom_visible ? "1" : "0") << "\n";
	file << "EncodingIndex=" << encoding_index << "\n";
	file << "WindowX=" << window_x << "\n";
	file << "WindowY=" << window_y << "\n";
	file << "WindowW=" << window_w << "\n";
	file << "WindowH=" << window_h << "\n";
	file << "WindowMaximized=" << (window_maximized ? "1" : "0") << "\n";
	for (size_t i = 0; i < column_widths.size(); ++i)
		file << "Column" << i << "=" << column_widths[i] << "\n";

	if (!spell_check_aff.empty() || !spell_check_dic.empty() || spell_lang_index >= 0)
	{
		file << "\n[SpellCheck]\n";
		if (!spell_check_aff.empty())
			file << "AffPath=" << spell_check_aff << "\n";
		if (!spell_check_dic.empty())
			file << "DicPath=" << spell_check_dic << "\n";
		file << "LangIndex=" << spell_lang_index << "\n";
	}

	if (!last_merge_order.empty())
	{
		file << "\n[MergeOrder]\n";
		file << "Count=" << last_merge_order.size() << "\n";
		for (size_t i = 0; i < last_merge_order.size(); ++i)
			file << "Path" << i << "=" << last_merge_order[i] << "\n";
	}
}
