#include "decoder/field_validator.hpp"
#include "decoder/scvr_condition.hpp"
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace field_validator {

constexpr unsigned long max_u8 = 255;
constexpr unsigned long max_u16 = 65535;
constexpr unsigned long max_u32 = 4294967295UL;
constexpr long min_i8 = -128;
constexpr long max_i8 = 127;
constexpr long min_i16 = -32768;
constexpr long max_i16 = 32767;
constexpr long min_i32 = -2147483648L;
constexpr long max_i32 = 2147483647L;
constexpr size_t max_string_var_bytes = 65535;

static validate_result_t make_valid()
{
	return { true, {} };
}

static validate_result_t make_invalid(const std::string & message)
{
	return { false, message };
}

static validate_result_t validate_unsigned(std::string_view input, unsigned long max_value)
{
	if (input.empty())
		return make_invalid("empty input");

	const std::string text(input);
	char * end_ptr = nullptr;
	errno = 0;
	const unsigned long value = std::strtoul(text.c_str(), &end_ptr, 10);

	if (end_ptr != text.c_str() + text.size())
		return make_invalid("not a valid unsigned integer");

	if (errno == ERANGE || value > max_value)
		return make_invalid("value out of range");

	return make_valid();
}

static validate_result_t validate_signed(std::string_view input, long min_value, long max_value)
{
	if (input.empty())
		return make_invalid("empty input");

	const std::string text(input);
	char * end_ptr = nullptr;
	errno = 0;
	const long value = std::strtol(text.c_str(), &end_ptr, 10);

	if (end_ptr != text.c_str() + text.size())
		return make_invalid("not a valid signed integer");

	if (errno == ERANGE || value < min_value || value > max_value)
		return make_invalid("value out of range");

	return make_valid();
}

static validate_result_t validate_signed_enum(
    std::string_view input,
    const char * const * enum_names,
    long min_value,
    long max_value)
{
	if (input.empty())
		return make_invalid("empty input");

	if (input == "None")
		return make_valid();

	for (const char * const * current = enum_names; *current != nullptr; ++current)
	{
		if (input == *current)
			return make_valid();
	}

	return validate_signed(input, min_value, max_value);
}

static validate_result_t validate_float(std::string_view input)
{
	if (input.empty())
		return make_invalid("empty input");

	const std::string text(input);
	char * end_ptr = nullptr;
	errno = 0;
	const float value = std::strtof(text.c_str(), &end_ptr);

	if (end_ptr != text.c_str() + text.size())
		return make_invalid("not a valid float");

	if (errno == ERANGE)
		return make_invalid("value out of range");

	if (std::isnan(value) || std::isinf(value))
		return make_invalid("NaN and Inf are not allowed");

	return make_valid();
}

static validate_result_t validate_string_fixed(std::string_view input, codepage_t codepage, size_t max_bytes)
{
	const std::string encoded = encode_from_utf8(std::string(input), codepage);

	if (encoded.size() > max_bytes)
		return make_invalid("encoded string exceeds field size");

	return make_valid();
}

static validate_result_t validate_string_var(std::string_view input, codepage_t codepage)
{
	const std::string encoded = encode_from_utf8(std::string(input), codepage);

	if (encoded.size() > max_string_var_bytes)
		return make_invalid("encoded string exceeds maximum size");

	return make_valid();
}

static validate_result_t validate_enum(std::string_view input, const char * const * enum_names)
{
	if (input.empty())
		return make_invalid("empty input");

	for (const char * const * current = enum_names; *current != nullptr; ++current)
	{
		if (input == *current)
			return make_valid();
	}

	return make_invalid("unknown enum value");
}

static bool is_hex_format(std::string_view input)
{
	return input.size() >= 3 && input[0] == '0' && (input[1] == 'x' || input[1] == 'X');
}

static validate_result_t validate_hex_flags(std::string_view input, unsigned long max_value)
{
	const std::string text(input);
	char * end_ptr = nullptr;
	errno = 0;
	const unsigned long value = std::strtoul(text.c_str(), &end_ptr, 16);

	if (end_ptr != text.c_str() + text.size())
		return make_invalid("not a valid hex value");

	if (errno == ERANGE || value > max_value)
		return make_invalid("hex value out of range");

	return make_valid();
}

static validate_result_t validate_flags(
    std::string_view input,
    const char * const * flag_names,
    unsigned long max_value)
{
	if (input.empty())
		return make_valid();

	if (is_hex_format(input))
		return validate_hex_flags(input, max_value);

	const std::string text(input);
	const std::string separator = " | ";
	size_t position = 0;

	while (position < text.size())
	{
		const size_t separator_pos = text.find(separator, position);
		const size_t token_end = (separator_pos == std::string::npos) ? text.size() : separator_pos;
		const std::string_view token(text.data() + position, token_end - position);

		if (token.empty())
			return make_invalid("empty flag name");

		bool found = false;
		for (const char * const * current = flag_names; *current != nullptr; ++current)
		{
			if (token == *current)
			{
				found = true;
				break;
			}
		}

		if (!found)
			return make_invalid("unknown flag name");

		if (separator_pos == std::string::npos)
			break;

		position = separator_pos + separator.size();
	}

	return make_valid();
}

static validate_result_t validate_bool_bit(std::string_view input)
{
	if (input == "Yes" || input == "No")
		return make_valid();

	return make_invalid("must be \"Yes\" or \"No\"");
}

static bool is_valid_hex_char(char character)
{
	return (character >= '0' && character <= '9') || (character >= 'A' && character <= 'F') ||
	       (character >= 'a' && character <= 'f');
}

static validate_result_t validate_hex_bytes(std::string_view input, size_t expected_bytes)
{
	if (input.empty())
	{
		if (expected_bytes == 0)
			return make_valid();

		return make_invalid("empty input");
	}

	size_t byte_count = 0;
	size_t position = 0;

	while (position < input.size())
	{
		if (position + 1 >= input.size())
			return make_invalid("incomplete hex byte pair");

		if (!is_valid_hex_char(input[position]) || !is_valid_hex_char(input[position + 1]))
			return make_invalid("invalid hex character");

		++byte_count;
		position += 2;

		if (position < input.size())
		{
			if (input[position] != ' ')
				return make_invalid("hex bytes must be space-separated");

			++position;
		}
	}

	if (byte_count != expected_bytes)
		return make_invalid("byte count does not match expected size");

	return make_valid();
}

validate_result_t validate_field(
    const field_def_t & field,
    std::string_view input,
    codepage_t codepage,
    size_t existing_sub_size)
{
	switch (field.type)
	{
	case field_type_t::u8:
		return validate_unsigned(input, max_u8);

	case field_type_t::u16:
		return validate_unsigned(input, max_u16);

	case field_type_t::u32:
		return validate_unsigned(input, max_u32);

	case field_type_t::i8:
		if (field.enum_names != nullptr)
			return validate_signed_enum(input, field.enum_names, min_i8, max_i8);

		return validate_signed(input, min_i8, max_i8);

	case field_type_t::i16:
		if (field.enum_names != nullptr)
			return validate_signed_enum(input, field.enum_names, min_i16, max_i16);

		return validate_signed(input, min_i16, max_i16);

	case field_type_t::i32:
		if (field.enum_names != nullptr)
			return validate_signed_enum(input, field.enum_names, min_i32, max_i32);

		return validate_signed(input, min_i32, max_i32);

	case field_type_t::f32:
		return validate_float(input);

	case field_type_t::string_fixed:
		return validate_string_fixed(input, codepage, field.size);

	case field_type_t::string_var:
		return validate_string_var(input, codepage);

	case field_type_t::enum_u8:
	case field_type_t::enum_u16:
	case field_type_t::enum_u32:
		return validate_enum(input, field.enum_names);

	case field_type_t::flags_u8:
		return validate_flags(input, field.flag_names, max_u8);

	case field_type_t::flags_u16:
		return validate_flags(input, field.flag_names, max_u16);

	case field_type_t::flags_u32:
		return validate_flags(input, field.flag_names, max_u32);

	case field_type_t::bool_bit:
		return validate_bool_bit(input);

	case field_type_t::binary:
		return validate_hex_bytes(input, field.size);

	case field_type_t::raw:
		return validate_hex_bytes(input, existing_sub_size);

	case field_type_t::scvr_type:
		return scvr_type_char(std::string(input)) != '\0' ? make_valid()
		                                                   : make_invalid("unknown condition type");

	case field_type_t::scvr_operator:
		return scvr_operator_char(std::string(input)) != '\0' ? make_valid()
		                                                       : make_invalid("unknown operator");
	}

	return make_invalid("unknown field type");
}

} // namespace field_validator
