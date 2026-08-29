#include "yaml_l10n_reader.hpp"
#include "yaml_scalar.hpp"
#include <fstream>
#include <sstream>

bool yaml_l10n_reader_t::load(const std::string & source_path, const std::string & target_path)
{
	m_source_entries = parse_yaml(source_path);
	if (m_source_entries.empty())
		return false;

	m_key_order.clear();
	for (const auto & entry : m_source_entries)
		m_key_order.push_back(entry.key);

	if (!target_path.empty())
		m_target_entries = parse_yaml(target_path);

	return true;
}

const std::vector<l10n_entry_t> & yaml_l10n_reader_t::source_entries() const
{
	return m_source_entries;
}

const std::vector<l10n_entry_t> & yaml_l10n_reader_t::target_entries() const
{
	return m_target_entries;
}

const std::vector<std::string> & yaml_l10n_reader_t::key_order() const
{
	return m_key_order;
}

std::string yaml_l10n_reader_t::read_block_scalar(
    std::ifstream & file,
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

	return block.str();
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
			entries.push_back({ key, yaml_scalar_t::decode_quoted(raw_value) });
			continue;
		}

		const auto chomp_mode = yaml_scalar_t::parse_block_indicator(raw_value);
		if (chomp_mode.has_value())
		{
			auto body = read_block_scalar(file, lookahead_line, has_lookahead);
			entries.push_back({ key, yaml_scalar_t::apply_chomp(std::move(body), chomp_mode.value()) });
			continue;
		}

		entries.push_back({ key, raw_value });
	}

	return entries;
}
