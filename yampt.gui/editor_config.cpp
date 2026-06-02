#include "editor_config.hpp"
#include <fstream>
#include <sstream>

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
	int base_dict_count = 0;

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

		if (section == "Editor")
		{
			if (key == "SplitRatio")
			{
				try { split_ratio = std::stof(value); } catch (...) {}
			}
			else if (key == "ColumnWidths")
			{
				size_t idx = 0;
				size_t pos = 0;
				while (idx < column_widths.size() && pos < value.size())
				{
					auto comma = value.find(',', pos);
					std::string token =
					    (comma == std::string::npos) ? value.substr(pos) : value.substr(pos, comma - pos);
					try { column_widths[idx] = std::stof(trim(token)); } catch (...) {}
					++idx;
					pos = (comma == std::string::npos) ? value.size() : comma + 1;
				}
			}
			else if (key == "SidebarWidth")
			{
				try { sidebar_width = std::stof(value); } catch (...) {}
			}
			else if (key == "BottomHeight")
			{
				try { bottom_height = std::stof(value); } catch (...) {}
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
				catch (...) {}
			}
			else if (key == "SpellCheckAff")
			{
				spell_check_aff = value;
			}
			else if (key == "SpellCheckDic")
			{
				spell_check_dic = value;
			}
		}
		else if (section == "BaseDicts")
		{
			if (key == "Count")
			{
				base_dict_count = std::stoi(value);
				base_dict_paths.clear();
				base_dict_paths.reserve(base_dict_count);
			}
			else if (starts_with(key, "Path"))
			{
				base_dict_paths.push_back(value);
			}
		}
		else if (section == "Recent")
		{
			if (key == "UserDict")
				last_user_dict_path = value;
			else if (key == "SourceDict")
				last_source_dict_path = value;
		}
	}
}

void editor_config_t::save(const std::string & path) const
{
	std::ofstream file(path);
	if (!file.is_open())
		return;

	file << "[Editor]\n";
	file << "SplitRatio=" << split_ratio << "\n";
	file << "ColumnWidths=";
	for (size_t i = 0; i < column_widths.size(); ++i)
	{
		if (i > 0)
			file << ",";
		file << column_widths[i];
	}
	file << "\n";
	file << "SidebarWidth=" << sidebar_width << "\n";
	file << "BottomHeight=" << bottom_height << "\n";
	file << "SidebarVisible=" << (sidebar_visible ? "1" : "0") << "\n";
	file << "BottomVisible=" << (bottom_visible ? "1" : "0") << "\n";
	file << "EncodingIndex=" << encoding_index << "\n";

	if (!spell_check_aff.empty())
		file << "SpellCheckAff=" << spell_check_aff << "\n";
	if (!spell_check_dic.empty())
		file << "SpellCheckDic=" << spell_check_dic << "\n";

	file << "\n[BaseDicts]\n";
	file << "Count=" << base_dict_paths.size() << "\n";
	for (size_t i = 0; i < base_dict_paths.size(); ++i)
		file << "Path" << i << "=" << base_dict_paths[i] << "\n";

	file << "\n[Recent]\n";
	if (!last_user_dict_path.empty())
		file << "UserDict=" << last_user_dict_path << "\n";
	if (!last_source_dict_path.empty())
		file << "SourceDict=" << last_source_dict_path << "\n";
}
