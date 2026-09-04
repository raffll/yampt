#include "string_utils.hpp"
#include "case_fold.hpp"
#include <cstdint>
#include <vector>

namespace string_utils {

namespace {

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

struct path_prefix_t
{
	std::string text;
	std::size_t remainder_start = 0;
	bool is_relative = true;
};

constexpr std::size_t drive_colon_position = 1;

bool starts_with_unc(std::string_view path)
{
	return path.size() >= 2 && path[0] == '/' && path[1] == '/' && (path.size() == 2 || path[2] != '/');
}

path_prefix_t parse_path_prefix(std::string_view path)
{
	path_prefix_t prefix;

	if (starts_with_unc(path))
	{
		const auto next_slash = path.find('/', 2);
		prefix.remainder_start = next_slash == std::string_view::npos ? path.size() : next_slash;
		prefix.text = std::string("//") + std::string(path.substr(2, prefix.remainder_start - 2));
		prefix.is_relative = false;
		return prefix;
	}

	if (path.size() > drive_colon_position && path[drive_colon_position] == ':')
	{
		prefix.text += static_cast<char>(std::toupper(static_cast<unsigned char>(path[0])));
		prefix.text += ':';
		prefix.remainder_start = 2;
		prefix.is_relative = false;
		return prefix;
	}

	if (!path.empty() && path[0] == '/')
	{
		prefix.remainder_start = 1;
		prefix.is_relative = false;
		return prefix;
	}

	return prefix;
}

std::vector<std::string> resolve_segments(std::string_view remainder, bool is_relative)
{
	std::vector<std::string> segments;
	std::size_t position = 0;

	while (position < remainder.size())
	{
		const auto next_slash = remainder.find('/', position);
		const auto end = next_slash == std::string_view::npos ? remainder.size() : next_slash;
		const auto segment = remainder.substr(position, end - position);
		position = end == remainder.size() ? end : end + 1;

		if (segment.empty() || segment == ".")
			continue;

		if (segment != "..")
		{
			segments.emplace_back(segment);
			continue;
		}

		if (!segments.empty() && segments.back() != "..")
		{
			segments.pop_back();
			continue;
		}

		if (is_relative)
			segments.emplace_back("..");
	}

	return segments;
}

std::string join_path(const path_prefix_t & prefix, const std::vector<std::string> & segments)
{
	std::string result = prefix.text;
	const bool prefix_has_root = !prefix.is_relative;

	for (std::size_t index = 0; index < segments.size(); ++index)
	{
		if (index > 0 || prefix_has_root)
			result += '/';

		result += segments[index];
	}

	if (result.empty() && prefix_has_root)
		result = "/";

	const bool is_drive_only = segments.empty() && !prefix.text.empty() && prefix.text.back() == ':';
	if (is_drive_only)
		result += '/';

	return result;
}

} // namespace

std::string canonicalize_path(std::string_view input)
{
	if (input.empty())
		return {};

	std::string forward_slashed(input);
	std::replace(forward_slashed.begin(), forward_slashed.end(), '\\', '/');

	const auto prefix = parse_path_prefix(forward_slashed);
	const std::string_view remainder = std::string_view(forward_slashed).substr(prefix.remainder_start);
	const auto segments = resolve_segments(remainder, prefix.is_relative);

	return join_path(prefix, segments);
}

bool paths_equal(std::string_view lhs, std::string_view rhs)
{
	const auto left = canonicalize_path(lhs);
	const auto right = canonicalize_path(rhs);

#ifdef _WIN32
	return to_lower(left) == to_lower(right);
#else
	return left == right;
#endif
}

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

		append_code_point(result, case_fold::to_lower(code_point));
		position += consumed;
	}

	return result;
}

bool case_insensitive_equal_utf8(std::string_view lhs, std::string_view rhs)
{
	return to_lower_utf8(lhs) == to_lower_utf8(rhs);
}

} // namespace string_utils
