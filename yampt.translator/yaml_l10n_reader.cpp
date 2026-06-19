#include "yaml_l10n_reader.hpp"
#include <fstream>
#include <sstream>

bool yaml_l10n_reader_t::load(const std::string & source_path, const std::string & target_path)
{
	source_entries_ = parse_yaml(source_path);
	if (source_entries_.empty())
		return false;

	key_order_.clear();
	for (const auto & entry : source_entries_)
		key_order_.push_back(entry.key);

	if (!target_path.empty())
		target_entries_ = parse_yaml(target_path);

	return true;
}

const std::vector<l10n_entry_t> & yaml_l10n_reader_t::source_entries() const
{
	return source_entries_;
}

const std::vector<l10n_entry_t> & yaml_l10n_reader_t::target_entries() const
{
	return target_entries_;
}

const std::vector<std::string> & yaml_l10n_reader_t::key_order() const
{
	return key_order_;
}

std::vector<l10n_entry_t> yaml_l10n_reader_t::parse_yaml(const std::string & path)
{
	std::vector<l10n_entry_t> entries;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return entries;

	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line.empty())
			continue;

		if (line[0] == '#')
			continue;

		auto delim_pos = line.find(": ");
		if (delim_pos == std::string::npos)
			continue;

		std::string key = line.substr(0, delim_pos);
		std::string raw_value = line.substr(delim_pos + 2);

		std::string value;

		if (!raw_value.empty() && raw_value[0] == '"')
		{
			auto close_pos = raw_value.find('"', 1);
			if (close_pos != std::string::npos)
				value = raw_value.substr(1, close_pos - 1);
			else
				value = raw_value.substr(1);
		}
		else if (raw_value == "|-" || raw_value == "|")
		{
			bool strip_trailing = (raw_value == "|-");
			std::ostringstream block;
			bool first_line = true;

			while (std::getline(file, line))
			{
				if (!line.empty() && line.back() == '\r')
					line.pop_back();

				if (line.empty() || (line[0] != ' ' && line[0] != '\t'))
				{
					if (!line.empty() && line[0] != '#')
					{
						auto d = line.find(": ");
						if (d != std::string::npos)
						{
							std::string next_key = line.substr(0, d);
							std::string next_raw = line.substr(d + 2);

							std::string assembled = block.str();
							if (strip_trailing && !assembled.empty() && assembled.back() == '\n')
								assembled.pop_back();

							entries.push_back({key, assembled});

							key = next_key;

							if (!next_raw.empty() && next_raw[0] == '"')
							{
								auto cp = next_raw.find('"', 1);
								if (cp != std::string::npos)
									value = next_raw.substr(1, cp - 1);
								else
									value = next_raw.substr(1);
							}
							else if (next_raw == "|-" || next_raw == "|")
							{
								strip_trailing = (next_raw == "|-");
								block.str("");
								block.clear();
								first_line = true;
								continue;
							}
							else
							{
								value = next_raw;
							}

							entries.push_back({key, value});
							goto next_entry;
						}
					}

					std::string assembled = block.str();
					if (strip_trailing && !assembled.empty() && assembled.back() == '\n')
						assembled.pop_back();

					entries.push_back({key, assembled});
					goto next_entry;
				}

				size_t indent = 0;
				while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t'))
					++indent;

				if (!first_line)
					block << '\n';

				block << line.substr(indent);
				first_line = false;
			}

			std::string assembled = block.str();
			if (strip_trailing && !assembled.empty() && assembled.back() == '\n')
				assembled.pop_back();

			entries.push_back({key, assembled});
			continue;
		}
		else
		{
			value = raw_value;
		}

		entries.push_back({key, value});

		next_entry:;
	}

	return entries;
}
