#pragma once

#include "../tools.hpp"
#include <string>
#include <cstddef>

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
	    : content_(record_content)
	{}

	bool next(sub_record_view_t & out)
	{
		if (pos_ + 8 > content_.size())
			return false;

		out.type = content_.substr(pos_, 4);
		out.offset = pos_;

		size_t declared_size = tools_t::convert_string_byte_array_to_uint(content_.substr(pos_ + 4, 4));
		out.size = (declared_size == 0) ? 1 : declared_size;

		if (pos_ + 8 + out.size > content_.size())
			return false;

		out.data = content_.data() + pos_ + 8;
		pos_ += 8 + out.size;
		return true;
	}

	void reset()
	{
		pos_ = 16;
	}

private:
	const std::string & content_;
	size_t pos_ = 16;
};
