#pragma once

#include <optional>
#include <string>
#include <string_view>

class yaml_scalar_t
{
public:
	enum class chomp_t
	{
		clip,
		strip,
		keep
	};

	static std::string decode_quoted(std::string_view raw_value);

	static std::optional<chomp_t> parse_block_indicator(std::string_view raw_value);

	static std::string apply_chomp(std::string body, chomp_t mode);
};
