#include "editor_config.hpp"
#include <fstream>

static std::string trim(const std::string & s)
{
	auto start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return {};
	auto end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

static bool starts_with(const std::string & line, const std::string & prefix)
{
	if (line.size() < prefix.size())
		return false;
	return line.compare(0, prefix.size(), prefix) == 0;
}

void editor_config_t::load(const std::string & path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return;

	std::string section;
	std::string line;

	while (std::getline(file, line))
	{
		line = trim(line);
		if (line.empty())
			continue;

		if (line.front() == '[' && line.back() == ']')
		{
			section = line.substr(1, line.size() - 2);
			continue;
		}

		auto eq_pos = line.find('=');
		if (eq_pos == std::string::npos)
			continue;

		std::string key = trim(line.substr(0, eq_pos));
		std::string value = trim(line.substr(eq_pos + 1));

		if (section == "UserDicts")
		{
			if (key == "Count")
			{
				int count = 0;
				try
				{
					count = std::stoi(value);
				}
				catch (...)
				{}
				user_dict_paths.clear();
				user_dict_paths.reserve(count);
			}
			else if (starts_with(key, "Path"))
			{
				user_dict_paths.push_back(value);
			}
		}
		else if (section == "BaseDicts")
		{
			if (key == "Count")
			{
				int count = 0;
				try
				{
					count = std::stoi(value);
				}
				catch (...)
				{}
				base_dict_paths.clear();
				base_dict_paths.reserve(count);
			}
			else if (starts_with(key, "Path"))
			{
				base_dict_paths.push_back(value);
			}
		}
		else if (section == "Editor")
		{
			if (key == "ActiveDictIndex")
			{
				try
				{
					active_dict_index = std::stoi(value);
				}
				catch (...)
				{}
			}
			else if (key == "SplitRatio")
			{
				try
				{
					split_ratio = std::stof(value);
				}
				catch (...)
				{}
			}
			else if (key == "SidebarWidth")
			{
				try
				{
					sidebar_width = std::stof(value);
				}
				catch (...)
				{}
			}
			else if (key == "BottomHeight")
			{
				try
				{
					bottom_height = std::stof(value);
				}
				catch (...)
				{}
			}
			else if (key == "InfoHeight")
			{
				try
				{
					info_height = std::stof(value);
				}
				catch (...)
				{}
			}
			else if (key == "SidebarVisible")
			{
				sidebar_visible = (value != "0");
			}
			else if (key == "BottomVisible")
			{
				bottom_visible = (value != "0");
			}
			else if (key == "EncodingIndex")
			{
				try
				{
					int idx = std::stoi(value);
					if (idx >= 0 && idx <= 2)
						encoding_index = idx;
				}
				catch (...)
				{}
			}
			else if (key == "WindowX")
			{
				try
				{
					window_x = std::stoi(value);
				}
				catch (...)
				{}
			}
			else if (key == "WindowY")
			{
				try
				{
					window_y = std::stoi(value);
				}
				catch (...)
				{}
			}
			else if (key == "WindowW")
			{
				try
				{
					window_w = std::stoi(value);
				}
				catch (...)
				{}
			}
			else if (key == "WindowH")
			{
				try
				{
					window_h = std::stoi(value);
				}
				catch (...)
				{}
			}
			else if (starts_with(key, "Column"))
			{
				try
				{
					int col_idx = std::stoi(key.substr(6));
					if (col_idx >= 0 && col_idx < static_cast<int>(column_widths.size()))
						column_widths[col_idx] = std::stof(value);
				}
				catch (...)
				{}
			}
		}
		else if (section == "SpellCheck")
		{
			if (key == "AffPath")
				spell_check_aff = value;
			else if (key == "DicPath")
				spell_check_dic = value;
			else if (key == "LangIndex")
			{
				try
				{
					spell_lang_index = std::stoi(value);
				}
				catch (...)
				{}
			}
		}
	}
}

void editor_config_t::save(const std::string & path) const
{
	std::ofstream file(path);
	if (!file.is_open())
		return;

	file << "[UserDicts]\n";
	file << "Count=" << user_dict_paths.size() << "\n";
	for (size_t i = 0; i < user_dict_paths.size(); ++i)
		file << "Path" << i << "=" << user_dict_paths[i] << "\n";

	file << "\n[BaseDicts]\n";
	file << "Count=" << base_dict_paths.size() << "\n";
	for (size_t i = 0; i < base_dict_paths.size(); ++i)
		file << "Path" << i << "=" << base_dict_paths[i] << "\n";

	file << "\n[Editor]\n";
	file << "ActiveDictIndex=" << active_dict_index << "\n";
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
}
