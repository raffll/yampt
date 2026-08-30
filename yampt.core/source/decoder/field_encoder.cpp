#include "decoder/field_encoder.hpp"
#include "decoder/scvr_condition.hpp"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace field_encoder {

static std::string encode_integer(std::string_view input, size_t width)
{
	int64_t value = 0;
	std::from_chars(input.data(), input.data() + input.size(), value);

	std::string result(width, '\0');
	std::memcpy(result.data(), &value, width);
	return result;
}

static std::string encode_unsigned(std::string_view input, size_t width)
{
	uint64_t value = 0;
	std::from_chars(input.data(), input.data() + input.size(), value);

	std::string result(width, '\0');
	std::memcpy(result.data(), &value, width);
	return result;
}

static std::string encode_float(std::string_view input)
{
	float value = std::strtof(std::string(input).c_str(), nullptr);

	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

static std::string encode_string_fixed(const encode_context_t & context)
{
	auto encoded = encode_from_utf8(std::string(context.input), context.codepage);

	std::string result(context.field.size, '\0');
	size_t copy_len = std::min(encoded.size(), context.field.size);
	std::memcpy(result.data(), encoded.data(), copy_len);
	return result;
}

static std::string encode_string_var(const encode_context_t & context)
{
	auto encoded = encode_from_utf8(std::string(context.input), context.codepage);

	bool original_has_null = false;
	if (context.existing_sub_size > 0 && context.existing_sub_data)
		original_has_null = (context.existing_sub_data[context.existing_sub_size - 1] == '\0');

	if (original_has_null)
		encoded.push_back('\0');

	return encoded;
}

static int find_enum_index(const char * const * enum_names, std::string_view name)
{
	for (int index = 0; enum_names[index] != nullptr; ++index)
	{
		if (name == enum_names[index])
			return index;
	}

	return -1;
}

static std::string encode_enum(const encode_context_t & context, size_t width)
{
	int index = find_enum_index(context.field.enum_names, context.input);

	std::string result(width, '\0');
	uint32_t value = static_cast<uint32_t>(index);
	std::memcpy(result.data(), &value, width);
	return result;
}

static std::string encode_signed_enum(const encode_context_t & context, size_t width)
{
	int32_t value = -1;
	if (context.input != "None")
		value = find_enum_index(context.field.enum_names, context.input);

	std::string result(width, '\0');
	std::memcpy(result.data(), &value, width);
	return result;
}

static std::string encode_bool_bit(const encode_context_t & context)
{
	uint8_t existing = 0;
	if (context.existing_sub_data && context.field.offset < context.existing_sub_size)
		std::memcpy(&existing, context.existing_sub_data + context.field.offset, 1);

	int bit_index = static_cast<int>(context.field.size);

	if (context.input == "Yes")
		existing |= static_cast<uint8_t>(1u << bit_index);
	else
		existing &= static_cast<uint8_t>(~(1u << bit_index));

	std::string result(1, '\0');
	result[0] = static_cast<char>(existing);
	return result;
}

static uint32_t parse_flags_hex(std::string_view input)
{
	uint32_t value = 0;
	auto start = input.data() + 2;
	std::from_chars(start, input.data() + input.size(), value, 16);
	return value;
}

static uint32_t compute_defined_mask(const field_def_t & field)
{
	uint32_t mask = 0;
	for (int bit_pos = 0; bit_pos < field.flag_count; ++bit_pos)
	{
		if (field.flag_names[bit_pos][0] != '_')
			mask |= (1u << bit_pos);
	}

	return mask;
}

static uint32_t parse_flag_names(const encode_context_t & context)
{
	uint32_t flags = 0;
	std::string_view remaining = context.input;

	while (!remaining.empty())
	{
		auto separator = remaining.find(" | ");
		std::string_view token;

		if (separator == std::string_view::npos)
		{
			token = remaining;
			remaining = {};
		}
		else
		{
			token = remaining.substr(0, separator);
			remaining = remaining.substr(separator + 3);
		}

		for (int bit_pos = 0; bit_pos < context.field.flag_count; ++bit_pos)
		{
			if (token == context.field.flag_names[bit_pos])
			{
				flags |= (1u << bit_pos);
				break;
			}
		}
	}

	return flags;
}

static std::string encode_flags(const encode_context_t & context, size_t width)
{
	uint32_t existing_value = 0;
	if (context.existing_sub_data && context.field.offset + width <= context.existing_sub_size)
		std::memcpy(&existing_value, context.existing_sub_data + context.field.offset, width);

	uint32_t new_flags = 0;

	if (context.input.empty())
	{
		new_flags = 0;
	}
	else if (context.input.size() > 2 && context.input[0] == '0' && context.input[1] == 'x')
	{
		new_flags = parse_flags_hex(context.input);
	}
	else
	{
		new_flags = parse_flag_names(context);
	}

	uint32_t defined_mask = compute_defined_mask(context.field);
	uint32_t hidden_bits = existing_value & ~defined_mask;
	uint32_t result_value = hidden_bits | new_flags;

	std::string result(width, '\0');
	std::memcpy(result.data(), &result_value, width);
	return result;
}

static std::string encode_hex_bytes(std::string_view input)
{
	std::string result;
	size_t offset = 0;

	while (offset < input.size())
	{
		while (offset < input.size() && input[offset] == ' ')
			++offset;

		if (offset + 1 >= input.size())
			break;

		uint8_t parsed_byte = 0;
		std::from_chars(input.data() + offset, input.data() + offset + 2, parsed_byte, 16);
		result.push_back(static_cast<char>(parsed_byte));
		offset += 2;
	}

	return result;
}

std::string encode_field(const encode_context_t & context)
{
	switch (context.field.type)
	{
	case field_type_t::u8:
		return encode_unsigned(context.input, 1);

	case field_type_t::u16:
		return encode_unsigned(context.input, 2);

	case field_type_t::u32:
		return encode_unsigned(context.input, 4);

	case field_type_t::i8:
		if (context.field.enum_names)
			return encode_signed_enum(context, 1);

		return encode_integer(context.input, 1);

	case field_type_t::i16:
		return encode_integer(context.input, 2);

	case field_type_t::i32:
		if (context.field.enum_names)
			return encode_signed_enum(context, 4);

		return encode_integer(context.input, 4);

	case field_type_t::f32:
		return encode_float(context.input);

	case field_type_t::string_fixed:
		return encode_string_fixed(context);

	case field_type_t::string_var:
		return encode_string_var(context);

	case field_type_t::enum_u8:
		return encode_enum(context, 1);

	case field_type_t::enum_u16:
		return encode_enum(context, 2);

	case field_type_t::enum_u32:
		return encode_enum(context, 4);

	case field_type_t::bool_bit:
		return encode_bool_bit(context);

	case field_type_t::flags_u8:
		return encode_flags(context, 1);

	case field_type_t::flags_u16:
		return encode_flags(context, 2);

	case field_type_t::flags_u32:
		return encode_flags(context, 4);

	case field_type_t::binary:
	case field_type_t::raw:
		return encode_hex_bytes(context.input);

	case field_type_t::scvr_type:
	{
		const char type_char = scvr_type_char(std::string(context.input));
		return std::string(1, type_char != '\0' ? type_char : '0');
	}

	case field_type_t::scvr_operator:
	{
		const char operator_char = scvr_operator_char(std::string(context.input));
		return std::string(1, operator_char != '\0' ? operator_char : '0');
	}

	case field_type_t::scvr_subject:
		return encode_string_fixed(context);
	}

	return {};
}

std::string patch_sub_record(
    const std::string & record_content,
    size_t sub_byte_offset,
    const field_def_t & field,
    const std::string & encoded_field)
{
	constexpr size_t sub_header_size = 8;
	std::string patched = record_content;

	if (field.type == field_type_t::string_var)
	{
		size_t data_start = sub_byte_offset + sub_header_size;
		size_t old_size = 0;
		std::memcpy(&old_size, patched.data() + sub_byte_offset + 4, 4);

		patched.replace(data_start, old_size, encoded_field);

		uint32_t new_size = static_cast<uint32_t>(encoded_field.size());
		std::memcpy(patched.data() + sub_byte_offset + 4, &new_size, 4);

		return patched;
	}

	size_t splice_pos = sub_byte_offset + sub_header_size + field.offset;
	patched.replace(splice_pos, encoded_field.size(), encoded_field);

	return patched;
}

std::string patch_record_size(const std::string & record_content)
{
	std::string patched = record_content;
	uint32_t new_size = static_cast<uint32_t>(patched.size() - 16);
	std::memcpy(patched.data() + 4, &new_size, 4);

	return patched;
}

} // namespace field_encoder
