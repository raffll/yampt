#include "exclusion_resolver.hpp"

namespace exclusion_resolver {
std::string regex_escape_literal(const std::string & id)
{
	static const std::string metacharacters = "\\^$.|?*+()[]{}";

	std::string result;
	result.reserve(id.size() * 2);

	for (const auto character : id)
	{
		if (metacharacters.find(character) != std::string::npos)
			result += '\\';

		result += character;
	}

	return result;
}
} // namespace exclusion_resolver

void exclusion_resolver_t::set_pattern(const std::string & pattern)
{
	if (pattern == m_pattern && (m_has_valid_pattern || pattern.empty()))
		return;

	m_pattern = pattern;
	m_has_valid_pattern = false;

	if (pattern.empty())
		return;

	try
	{
		m_regex = std::regex(pattern, std::regex::icase);
		m_has_valid_pattern = true;
	}
	catch (...)
	{
		m_has_valid_pattern = false;
	}
}

void exclusion_resolver_t::set_disabled_types(const std::set<std::string> & disabled_types)
{
	m_disabled_types = disabled_types;
}

bool exclusion_resolver_t::is_record_excluded(const std::string & rec_type, const std::string & record_id) const
{
	if (is_type_disabled(rec_type))
		return true;

	return matches_pattern(record_id);
}

bool exclusion_resolver_t::matches_pattern(const std::string & record_id) const
{
	if (!m_has_valid_pattern)
		return false;

	return std::regex_search(record_id, m_regex);
}

bool exclusion_resolver_t::is_type_disabled(const std::string & rec_type) const
{
	return m_disabled_types.count(rec_type) != 0;
}
