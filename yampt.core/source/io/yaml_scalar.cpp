#include "yaml_scalar.hpp"

std::string yaml_scalar_t::decode_quoted(std::string_view raw_value)
{
	std::string result;
	result.reserve(raw_value.size());

	for (size_t index = 1; index < raw_value.size(); ++index)
	{
		const char current = raw_value[index];

		if (current == '"')
			break;

		if (current != '\\' || index + 1 >= raw_value.size())
		{
			result += current;
			continue;
		}

		const char next_char = raw_value[index + 1];
		++index;

		switch (next_char)
		{
		case 'n':
			result += '\n';
			break;

		case 't':
			result += '\t';
			break;

		case 'r':
			result += '\r';
			break;

		case '0':
			result += '\0';
			break;

		case '"':
			result += '"';
			break;

		case '\\':
			result += '\\';
			break;

		default:
			result += '\\';
			result += next_char;
			break;
		}
	}

	return result;
}

std::optional<yaml_scalar_t::chomp_t> yaml_scalar_t::parse_block_indicator(std::string_view raw_value)
{
	if (raw_value == "|")
		return chomp_t::clip;

	if (raw_value == "|-")
		return chomp_t::strip;

	if (raw_value == "|+")
		return chomp_t::keep;

	return std::nullopt;
}

std::string yaml_scalar_t::apply_chomp(std::string body, chomp_t mode)
{
	if (mode == chomp_t::strip)
	{
		while (!body.empty() && body.back() == '\n')
			body.pop_back();

		return body;
	}

	if (mode == chomp_t::keep)
	{
		if (body.empty() || body.back() != '\n')
			body += '\n';

		return body;
	}

	return body;
}
