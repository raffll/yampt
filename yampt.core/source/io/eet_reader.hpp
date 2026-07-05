#pragma once

#include "../utility/includes.hpp"
#include <cstdint>

class eet_reader_t
{
public:
	struct eet_entry_t
	{
		std::string rec_type;
		std::string sub_type;
		std::string context;
		std::string key_text;
		std::string orig;
		std::string trans;
		uint8_t status_byte = 0xFF;
	};

	bool load(const std::string & path);

	const auto & entries() const
	{
		return m_entries;
	}

private:
	bool parse_header(const std::string & content, size_t & offset);
	bool parse_header_game_and_line(const std::string & content, size_t & offset);
	bool parse_entry(const std::string & content, size_t & offset);
	std::string read_length_prefixed_string(const std::string & content, size_t & offset);
	uint32_t read_uint32(const std::string & content, size_t offset) const;
	uint16_t read_uint16(const std::string & content, size_t offset) const;

	std::vector<eet_entry_t> m_entries;
	uint32_t m_entry_count = 0;
};
