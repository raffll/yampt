#pragma once

#include "decoder/sub_record_schema.hpp"
#include "io/codepage.hpp"

#include <string>
#include <string_view>

namespace field_validator
{

struct validate_result_t
{
	bool valid;
	std::string error_message;
};

validate_result_t validate_field(const field_def_t & field,
                                 std::string_view input,
                                 codepage_t codepage,
                                 size_t existing_sub_size);

} // namespace field_validator
