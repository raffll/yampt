#include "yaml_l10n_writer.hpp"
#include "yaml_l10n_reader.hpp"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

static bool needs_quoting(const std::string & value)
{
	if (value.empty())
		return true;

	static const std::unordered_set<std::string> reserved_words = {
		"true", "false", "yes", "no", "on", "off", "null", "~",
		"True", "False", "Yes", "No", "On", "Off", "Null",
		"TRUE", "FALSE", "YES", "NO", "ON", "OFF", "NULL"
	};

	if (reserved_words.count(value))
		return true;

	if (value[0] == ' ' || value.back() == ' ')
		return true;

	if (value.find(':') != std::string::npos || value[0] == '{' || value[0] == '[' || value[0] == '&' ||
	    value[0] == '*' || value[0] == '?' || value[0] == '|' || value[0] == '-' || value[0] == '<' ||
	    value[0] == '>' || value[0] == '!' || value[0] == '%' || value[0] == '@' || value[0] == '#' ||
	    value[0] == '\'' || value[0] == '"')
		return true;

	bool looks_numeric = true;
	bool has_dot = false;
	for (size_t index = 0; index < value.size(); ++index)
	{
		char character = value[index];
		if (index == 0 && (character == '+' || character == '-'))
			continue;

		if (character == '.' && !has_dot)
		{
			has_dot = true;
			continue;
		}

		if (character < '0' || character > '9')
		{
			looks_numeric = false;
			break;
		}
	}

	if (looks_numeric && !value.empty() && value != "-" && value != "+")
		return true;

	return false;
}

bool yaml_l10n_writer_t::write(
    const std::string & output_path,
    const std::vector<l10n_entry_t> & entries,
    const std::vector<std::string> & key_order)
{
	std::ofstream file(output_path, std::ios::binary);
	if (!file.is_open())
		return false;

	std::unordered_map<std::string, std::string> value_map;
	for (const auto & entry : entries)
		value_map[entry.key] = entry.value;

	for (const auto & key : key_order)
	{
		auto it = value_map.find(key);
		if (it == value_map.end())
			continue;

		const auto & value = it->second;

		if (value.find('\n') != std::string::npos)
		{
			file << key << ": |-\n";

			size_t pos = 0;
			while (pos < value.size())
			{
				auto nl = value.find('\n', pos);
				if (nl == std::string::npos)
				{
					file << "  " << value.substr(pos) << "\n";
					break;
				}

				file << "  " << value.substr(pos, nl - pos) << "\n";
				pos = nl + 1;
			}

			continue;
		}

		if (needs_quoting(value))
		{
			std::string escaped;
			escaped.reserve(value.size());
			for (char ch : value)
			{
				if (ch == '"')
					escaped += "\\\"";
				else if (ch == '\\')
					escaped += "\\\\";
				else
					escaped += ch;
			}

			file << key << ": \"" << escaped << "\"\n";
			continue;
		}

		file << key << ": " << value << "\n";
	}

	return true;
}
