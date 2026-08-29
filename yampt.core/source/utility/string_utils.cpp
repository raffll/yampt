#include "string_utils.hpp"
#include "string_utils.hpp"
#include <cstdint>

namespace string_utils {

namespace {

std::uint32_t fold_code_point(std::uint32_t code_point)
{
	if (code_point >= 'A' && code_point <= 'Z')
		return code_point + 0x20;

	if (code_point >= 0x00C0 && code_point <= 0x00DE && code_point != 0x00D7)
		return code_point + 0x20;

	if (code_point >= 0x0139 && code_point <= 0x0148)
	{
		if (code_point % 2 == 1)
			return code_point + 1;

		return code_point;
	}

	if (code_point >= 0x0100 && code_point <= 0x0177)
	{
		if (code_point % 2 == 0)
			return code_point + 1;

		return code_point;
	}

	if (code_point == 0x0178)
		return 0x00FF;

	if (code_point >= 0x0179 && code_point <= 0x017E)
	{
		if (code_point % 2 == 1)
			return code_point + 1;

		return code_point;
	}

	if (code_point >= 0x0410 && code_point <= 0x042F)
		return code_point + 0x20;

	if (code_point == 0x0401)
		return 0x0451;

	return code_point;
}

int decode_utf8(const unsigned char * bytes, int remaining, std::uint32_t & code_point)
{
	const unsigned char lead = bytes[0];

	if (lead < 0x80)
	{
		code_point = lead;
		return 1;
	}

	if ((lead & 0xE0) == 0xC0 && remaining >= 2 && (bytes[1] & 0xC0) == 0x80)
	{
		code_point = ((lead & 0x1Fu) << 6) | (bytes[1] & 0x3Fu);
		return 2;
	}

	if ((lead & 0xF0) == 0xE0 && remaining >= 3 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80)
	{
		code_point = ((lead & 0x0Fu) << 12) | ((bytes[1] & 0x3Fu) << 6) | (bytes[2] & 0x3Fu);
		return 3;
	}

	if ((lead & 0xF8) == 0xF0 && remaining >= 4 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80 &&
	    (bytes[3] & 0xC0) == 0x80)
	{
		code_point = ((lead & 0x07u) << 18) | ((bytes[1] & 0x3Fu) << 12) | ((bytes[2] & 0x3Fu) << 6) |
		    (bytes[3] & 0x3Fu);
		return 4;
	}

	code_point = lead;
	return -1;
}

void append_code_point(std::string & out, std::uint32_t code_point)
{
	if (code_point < 0x80)
	{
		out += static_cast<char>(code_point);
		return;
	}

	if (code_point < 0x800)
	{
		out += static_cast<char>(0xC0 | (code_point >> 6));
		out += static_cast<char>(0x80 | (code_point & 0x3F));
		return;
	}

	if (code_point < 0x10000)
	{
		out += static_cast<char>(0xE0 | (code_point >> 12));
		out += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (code_point & 0x3F));
		return;
	}

	out += static_cast<char>(0xF0 | (code_point >> 18));
	out += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
	out += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
	out += static_cast<char>(0x80 | (code_point & 0x3F));
}

} // namespace

std::string to_lower_utf8(std::string_view input)
{
	std::string result;
	result.reserve(input.size());

	const auto * bytes = reinterpret_cast<const unsigned char *>(input.data());
	int position = 0;
	const auto total = static_cast<int>(input.size());

	while (position < total)
	{
		std::uint32_t code_point = 0;
		const int consumed = decode_utf8(bytes + position, total - position, code_point);

		if (consumed < 0)
		{
			result += static_cast<char>(bytes[position]);
			position += 1;
			continue;
		}

		append_code_point(result, fold_code_point(code_point));
		position += consumed;
	}

	return result;
}

bool case_insensitive_equal_utf8(std::string_view lhs, std::string_view rhs)
{
	return to_lower_utf8(lhs) == to_lower_utf8(rhs);
}

} // namespace string_utils
