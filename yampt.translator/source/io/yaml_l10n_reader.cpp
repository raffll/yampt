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

std::string yaml_l10n_reader_t::parse_quoted_value(const std::string & raw_value) const
{
	auto close_pos = raw_value.find('"', 1);
	if (close_pos != std::string::npos)
		return raw_value.substr(1, close_pos - 1);

	return raw_value.substr(1);
}

std::string yaml_l10n_reader_t::read_block_scalar(
    std::ifstream & file,
    bool strip_trailing,
    std::string & lookahead_line,
    bool & has_lookahead) const
{
	std::ostringstream block;
	bool first_line = true;
	std::string line;

	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line.empty() || (line[0] != ' ' && line[0] != '\t'))
		{
			lookahead_line = line;
			has_lookahead = true;
			break;
		}

		size_t indent = 0;
		while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t'))
			++indent;

		if (!first_line)
			block << '\n';

		block << line.substr(indent);
		first_line = false;
	}

	auto assembled = block.str();
	if (strip_trailing && !assembled.empty() && assembled.back() == '\n')
		assembled.pop_back();

	return assembled;
}

std::vector<l10n_entry_t> yaml_l10n_reader_t::parse_yaml(const std::string & path)
{
	std::vector<l10n_entry_t> entries;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
		return entries;

	std::string line;
	bool has_lookahead = false;
	std::string lookahead_line;

	while (has_lookahead || std::getline(file, line))
	{
		if (has_lookahead)
		{
			line = lookahead_line;
			has_lookahead = false;
		}

		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line.empty() || line[0] == '#')
			continue;

		auto delim_pos = line.find(": ");
		if (delim_pos == std::string::npos)
			continue;

		std::string key = line.substr(0, delim_pos);
		std::string raw_value = line.substr(delim_pos + 2);

		if (!raw_value.empty() && raw_value[0] == '"')
		{
			entries.push_back({ key, parse_quoted_value(raw_value) });
			continue;
		}

		if (raw_value == "|-" || raw_value == "|")
		{
			bool strip_trailing = (raw_value == "|-");
			auto value = read_block_scalar(file, strip_trailing, lookahead_line, has_lookahead);
			entries.push_back({ key, value });
			continue;
		}

		entries.push_back({ key, raw_value });
	}

	return entries;
}
