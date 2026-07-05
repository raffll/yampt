#pragma once

#include "../utility/tools.hpp"
#include <cstddef>
#include <cstring>
#include <string>

static constexpr uint32_t frmr_ref_index_mask = 0x00FFFFFF;

inline uint32_t read_frmr_ref_index(const char * data, size_t size)
{
	uint32_t raw_value = 0;
	if (size >= 4)
		std::memcpy(&raw_value, data, 4);

	return raw_value & frmr_ref_index_mask;
}

struct sub_record_view_t
{
	std::string type;
	const char * data;
	size_t size;
	size_t offset;
};

class sub_record_iter_t
{
public:
	explicit sub_record_iter_t(const std::string & record_content)
	    : m_content(record_content)
	{}

	bool next(sub_record_view_t & out)
	{
		if (m_pos + 8 > m_content.size())
			return false;

		out.type = m_content.substr(m_pos, 4);
		out.offset = m_pos;

		size_t declared_size = tools_t::convert_string_byte_array_to_uint(m_content.substr(m_pos + 4, 4));
		out.size = (declared_size == 0) ? 1 : declared_size;

		if (m_pos + 8 + out.size > m_content.size())
			return false;

		out.data = m_content.data() + m_pos + 8;
		m_pos += 8 + out.size;
		return true;
	}

	void reset()
	{
		m_pos = 16;
	}

private:
	const std::string & m_content;
	size_t m_pos = 16;
};
