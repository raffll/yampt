#include "eet_reader.hpp"
#include "../utility/tools.hpp"

bool eet_reader_t::load(const std::string & path)
{
	m_entries.clear();
	m_entry_count = 0;

	const auto content = tools_t::read_file(path);
	if (content.empty())
	{
		tools_t::add_log("[error] cannot read EET file: \"" + path + "\"\r\n");
		return false;
	}

	size_t offset = 0;

	if (!parse_header(content, offset))
		return false;

	m_entries.reserve(m_entry_count);

	for (uint32_t index = 0; index < m_entry_count; ++index)
	{
		if (!parse_entry(content, offset))
		{
			tools_t::add_log("[error] EET parse stopped at entry " + std::to_string(index)
				+ " of " + std::to_string(m_entry_count) + "\r\n");
			return false;
		}
	}

	tools_t::add_log("[info] loaded EET file: " + std::to_string(m_entries.size())
		+ " entries from \"" + path + "\"\r\n");

	return true;
}

bool eet_reader_t::parse_header(const std::string & content, size_t & offset)
{
	constexpr size_t magic_size = 4;
	constexpr size_t version_size = 4;
	constexpr size_t unknown_size = 4;
	constexpr size_t game_tag_size = 4;
	constexpr size_t game_name_len_size = 2;
	constexpr size_t line_tag_size = 4;
	constexpr size_t entry_count_size = 4;
	constexpr size_t minimum_header = magic_size + version_size + unknown_size
		+ game_tag_size + game_name_len_size + line_tag_size + entry_count_size;

	if (content.size() < minimum_header)
	{
		tools_t::add_log("[error] invalid EET file: too short for header\r\n");
		return false;
	}

	const auto magic = content.substr(offset, magic_size);
	if (magic != "EET_")
	{
		tools_t::add_log("[error] invalid EET file: bad magic\r\n");
		return false;
	}
	offset += magic_size;

	offset += version_size;
	offset += unknown_size;

	return parse_header_game_and_line(content, offset);
}

bool eet_reader_t::parse_header_game_and_line(const std::string & content, size_t & offset)
{
	constexpr size_t game_tag_size = 4;
	constexpr size_t game_name_len_size = 2;
	constexpr size_t line_tag_size = 4;
	constexpr size_t entry_count_size = 4;

	const auto game_tag = content.substr(offset, game_tag_size);
	if (game_tag != "GAME")
	{
		tools_t::add_log("[error] invalid EET file: bad game tag\r\n");
		return false;
	}
	offset += game_tag_size;

	const auto game_name_length = read_uint16(content, offset);
	offset += game_name_len_size;

	if (offset + game_name_length > content.size())
	{
		tools_t::add_log("[error] invalid EET file: truncated game name\r\n");
		return false;
	}
	offset += game_name_length;

	const auto line_tag = content.substr(offset, line_tag_size);
	if (line_tag != "LINE")
	{
		tools_t::add_log("[error] invalid EET file: bad line tag\r\n");
		return false;
	}
	offset += line_tag_size;

	if (offset + entry_count_size > content.size())
	{
		tools_t::add_log("[error] invalid EET file: truncated entry count\r\n");
		return false;
	}

	m_entry_count = read_uint32(content, offset);
	offset += entry_count_size;

	return true;
}

bool eet_reader_t::parse_entry(const std::string & content, size_t & offset)
{
	constexpr size_t total_size_field = 4;
	constexpr size_t tail_status_offset = 8;

	if (offset + total_size_field > content.size())
		return false;

	const auto total_size = read_uint32(content, offset);
	offset += total_size_field;

	const auto entry_end = offset + total_size;
	if (entry_end > content.size())
		return false;

	eet_entry_t entry;
	entry.rec_type = read_length_prefixed_string(content, offset);
	entry.context = read_length_prefixed_string(content, offset);
	entry.key_text = read_length_prefixed_string(content, offset);
	entry.sub_type = read_length_prefixed_string(content, offset);
	entry.orig = read_length_prefixed_string(content, offset);
	entry.trans = read_length_prefixed_string(content, offset);

	const auto tail_start = offset;
	const auto tail_size = entry_end - tail_start;

	if (tail_size > tail_status_offset)
		entry.status_byte = static_cast<uint8_t>(content[tail_start + tail_status_offset]);

	offset = entry_end;
	m_entries.push_back(std::move(entry));

	return true;
}

std::string eet_reader_t::read_length_prefixed_string(const std::string & content, size_t & offset)
{
	constexpr size_t length_field_size = 4;

	if (offset + length_field_size > content.size())
		return {};

	const auto length = read_uint32(content, offset);
	offset += length_field_size;

	if (offset + length > content.size())
		return {};

	auto result = content.substr(offset, length);
	offset += length;

	return result;
}

uint32_t eet_reader_t::read_uint32(const std::string & content, size_t offset) const
{
	return static_cast<uint8_t>(content[offset])
		| (static_cast<uint8_t>(content[offset + 1]) << 8)
		| (static_cast<uint8_t>(content[offset + 2]) << 16)
		| (static_cast<uint8_t>(content[offset + 3]) << 24);
}

uint16_t eet_reader_t::read_uint16(const std::string & content, size_t offset) const
{
	return static_cast<uint8_t>(content[offset])
		| (static_cast<uint8_t>(content[offset + 1]) << 8);
}
