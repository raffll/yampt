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

		if (section == "WorkspaceRoots")
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
				workspace_roots.clear();
				workspace_roots.reserve(count);
			}
			else if (starts_with(key, "Path"))
			{
				workspace_roots.push_back(value);
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
			else if (key == "ActiveDictPath")
			{
				active_dict_path = value;
			}
			else if (key == "DeepLApiKey")
			{
				deepl_api_key = value;
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
			else if (key == "WindowMaximized")
			{
				window_maximized = (value != "0");
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
		else if (section == "MergeOrder")
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
				last_merge_order.clear();
				last_merge_order.reserve(count);
			}
			else if (starts_with(key, "Path"))
			{
				last_merge_order.push_back(value);
			}
		}
	}
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
