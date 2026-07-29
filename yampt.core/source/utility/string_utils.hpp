#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace string_utils {

inline std::string to_lower(std::string_view input)
{
	std::string result(input);
	std::transform(
	    result.begin(),
	    result.end(),
	    result.begin(),
	    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

inline std::string normalize_path(std::string_view input)
{
	std::string result(input);
	std::replace(result.begin(), result.end(), '\\', '/');
	return result;
}

inline std::string_view extract_filename(std::string_view path)
{
	const auto pos = path.find_last_of("\\/");
	if (pos == std::string_view::npos)
		return path;

	return path.substr(pos + 1);
}

inline std::string_view trim(std::string_view input)
{
	const auto start = input.find_first_not_of(" \t\r\n");
	if (start == std::string_view::npos)
		return {};

	const auto end = input.find_last_not_of(" \t\r\n");
	return input.substr(start, end - start + 1);
}

inline int utf8_byte_to_char_offset(const std::string & utf8_text, int byte_offset)
{
	int char_count = 0;
	int byte_count = 0;
	const auto * bytes = reinterpret_cast<const unsigned char *>(utf8_text.data());
	const auto total = static_cast<int>(utf8_text.size());

	while (byte_count < byte_offset && byte_count < total)
	{
		const unsigned char lead = bytes[byte_count];
		if (lead < 0x80)
			byte_count += 1;
		else if ((lead & 0xE0) == 0xC0)
			byte_count += 2;
		else if ((lead & 0xF0) == 0xE0)
			byte_count += 3;
		else
			byte_count += 4;

		++char_count;
	}

	return char_count;
}

inline bool case_insensitive_equal(std::string_view lhs, std::string_view rhs)
{
	return to_lower(lhs) == to_lower(rhs);
}

inline std::string erase_null_chars(std::string str)
{
	const auto pos = str.find('\0');
	if (pos != std::string::npos)
		str.erase(pos);

	return str;
}

inline std::string trim_cr(std::string str)
{
	if (!str.empty() && str.back() == '\r')
		str.pop_back();

	return str;
}

inline std::string replace_non_printable_with_dot(const std::string & str)
{
	std::string result;
	result.reserve(str.size());
	for (const auto ch : str)
	{
		if (std::isprint(static_cast<unsigned char>(ch)))
			result += ch;
		else
			result += '.';
	}
	return result;
}

} // namespace string_utils
