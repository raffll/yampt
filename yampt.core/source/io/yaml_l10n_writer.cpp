#include "yaml_l10n_writer.hpp"
#include "yaml_l10n_reader.hpp"
#include <fstream>
#include <unordered_map>

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

		if (!value.empty() &&
		    (value.find(':') != std::string::npos || value[0] == '{' || value[0] == '[' || value[0] == '&' ||
		     value[0] == '*' || value[0] == '?' || value[0] == '|' || value[0] == '-' || value[0] == '<' ||
		     value[0] == '>' || value[0] == '!' || value[0] == '%' || value[0] == '@' || value[0] == '#' ||
		     value[0] == '\'' || value[0] == '"'))
		{
			file << key << ": \"" << value << "\"\n";
			continue;
		}

		file << key << ": " << value << "\n";
	}

	return true;
}
