#pragma once

#include <regex>
#include <set>
#include <string>
#include <vector>

namespace merge_exclusions {
std::string regex_escape_literal(const std::string & text);
std::string anchored_id_token(const std::string & record_id);
} // namespace merge_exclusions

enum class exclude_kind_t
{
	file,
	record_id,
	record_type,
	sub_record
};

struct exclude_rule_t
{
	exclude_kind_t kind = exclude_kind_t::record_id;
	std::string target;

	bool operator==(const exclude_rule_t &) const = default;
};

class merge_exclusions_t
{
public:
	void set_rules(const std::vector<exclude_rule_t> & rules);
	const std::vector<exclude_rule_t> & rules() const;

	bool is_file_excluded(const std::string & filename) const;
	bool is_type_excluded(const std::string & rec_type) const;
	bool is_record_excluded(const std::string & rec_type, const std::string & record_id) const;

	std::set<std::string> ignored_sub_records() const;

	static std::vector<exclude_rule_t> parse(const std::string & serialized);
	static std::string serialize(const std::vector<exclude_rule_t> & rules);
	static const char * kind_token(exclude_kind_t kind);

private:
	void compile();

	std::vector<exclude_rule_t> m_rules;
	std::set<std::string> m_excluded_files;
	std::set<std::string> m_excluded_types;
	std::vector<std::regex> m_id_regexes;
};
