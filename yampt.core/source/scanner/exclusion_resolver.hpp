#pragma once

#include <regex>
#include <set>
#include <string>

namespace exclusion_resolver {
std::string regex_escape_literal(const std::string & id);
} // namespace exclusion_resolver

class exclusion_resolver_t
{
public:
	void set_pattern(const std::string & pattern);
	void set_disabled_types(const std::set<std::string> & disabled_types);

	bool is_record_excluded(const std::string & rec_type, const std::string & record_id) const;

private:
	bool matches_pattern(const std::string & record_id) const;
	bool is_type_disabled(const std::string & rec_type) const;

	std::string m_pattern;
	std::regex m_regex;
	bool m_has_valid_pattern = false;
	std::set<std::string> m_disabled_types;
};
