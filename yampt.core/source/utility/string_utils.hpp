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

} // namespace string_utils
