#pragma once

#include "decoder/sub_record_schema.hpp"
#include "io/codepage.hpp"
#include <string>
#include <string_view>

namespace field_encoder {

struct encode_context_t
{
	const field_def_t & field;
	std::string_view input;
	codepage_t codepage;
	const char * existing_sub_data;
	size_t existing_sub_size;
};

std::string encode_field(const encode_context_t & context);

std::string patch_sub_record(
    const std::string & record_content,
    size_t sub_byte_offset,
    const field_def_t & field,
    const std::string & encoded_field);

std::string patch_record_size(const std::string & record_content);

} // namespace field_encoder
