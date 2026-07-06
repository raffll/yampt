#pragma once

#include "../utility/domain_types.hpp"
#include "eet_reader.hpp"

class eet_converter_t
{
public:
	explicit eet_converter_t(const std::vector<eet_reader_t::eet_entry_t> & entries);

	const dict_t & get_dict() const
	{
		return m_dict;
	}

	size_t converted_count() const
	{
		return m_converted_count;
	}

	size_t skipped_count() const
	{
		return m_skipped_count;
	}

private:
	rec_type_t map_type(const std::string & rec_type, const std::string & sub_type) const;
	status_t map_status(uint8_t status_byte) const;
	std::string build_key_text(const eet_reader_t::eet_entry_t & entry, rec_type_t yampt_type) const;

	dict_t m_dict;
	size_t m_converted_count = 0;
	size_t m_skipped_count = 0;
};
