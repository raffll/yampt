#include "merge_exclusions.hpp"
#include "../utility/string_utils.hpp"

namespace {
constexpr char rule_separator = '\n';
constexpr char kind_separator = ':';

exclude_kind_t kind_from_token(std::string_view token)
{
	if (token == "file")
		return exclude_kind_t::file;

	if (token == "type")
		return exclude_kind_t::record_type;

	if (token == "sub")
		return exclude_kind_t::sub_record;

	return exclude_kind_t::record_id;
}
} // namespace

namespace merge_exclusions {
std::string regex_escape_literal(const std::string & text)
{
	static const std::string metacharacters = "\\^$.|?*+()[]{}";

	std::string result;
	result.reserve(text.size() * 2);

	for (const auto character : text)
	{
		if (metacharacters.find(character) != std::string::npos)
			result += '\\';

		result += character;
	}

	return result;
}

std::string anchored_id_token(const std::string & record_id)
{
	return "^" + regex_escape_literal(record_id) + "$";
}
} // namespace merge_exclusions

const char * merge_exclusions_t::kind_token(exclude_kind_t kind)
{
	switch (kind)
	{
	case exclude_kind_t::file:
		return "file";

	case exclude_kind_t::record_type:
		return "type";

	case exclude_kind_t::sub_record:
		return "sub";

	case exclude_kind_t::record_id:
		return "id";
	}

	return "id";
}

std::vector<exclude_rule_t> merge_exclusions_t::parse(const std::string & serialized)
{
	std::vector<exclude_rule_t> result;

	size_t start = 0;
	while (start <= serialized.size())
	{
		const auto pos = serialized.find(rule_separator, start);
		const auto end = (pos == std::string::npos) ? serialized.size() : pos;
		const auto line = std::string(string_utils::trim(std::string_view(serialized).substr(start, end - start)));
		start = end + 1;

		if (line.empty())
		{
			if (pos == std::string::npos)
				break;

			continue;
		}

		const auto sep = line.find(kind_separator);
		if (sep == std::string::npos)
			continue;

		const auto kind_text = string_utils::trim(std::string_view(line).substr(0, sep));
		const auto target = std::string(string_utils::trim(std::string_view(line).substr(sep + 1)));
		if (target.empty())
			continue;

		result.push_back({ kind_from_token(kind_text), target });

		if (pos == std::string::npos)
			break;
	}

	return result;
}

std::string merge_exclusions_t::serialize(const std::vector<exclude_rule_t> & rules)
{
	std::string result;
	for (const auto & rule : rules)
	{
		result += kind_token(rule.kind);
		result += kind_separator;
		result += rule.target;
		result += rule_separator;
	}

	return result;
}

void merge_exclusions_t::set_rules(const std::vector<exclude_rule_t> & rules)
{
	m_rules = rules;
	compile();
}

const std::vector<exclude_rule_t> & merge_exclusions_t::rules() const
{
	return m_rules;
}

void merge_exclusions_t::compile()
{
	m_excluded_files.clear();
	m_excluded_types.clear();
	m_id_regexes.clear();

	for (const auto & rule : m_rules)
	{
		if (rule.kind == exclude_kind_t::file)
		{
			m_excluded_files.insert(string_utils::to_lower(rule.target));
			continue;
		}

		if (rule.kind == exclude_kind_t::record_type)
		{
			m_excluded_types.insert(rule.target);
			continue;
		}

		if (rule.kind == exclude_kind_t::record_id)
		{
			try
			{
				m_id_regexes.emplace_back(rule.target, std::regex::icase);
			}
			catch (...)
			{
			}

			continue;
		}
	}
}

bool merge_exclusions_t::is_file_excluded(const std::string & filename) const
{
	return m_excluded_files.count(string_utils::to_lower(filename)) != 0;
}

bool merge_exclusions_t::is_type_excluded(const std::string & rec_type) const
{
	return m_excluded_types.count(rec_type) != 0;
}

bool merge_exclusions_t::is_record_excluded(const std::string & rec_type, const std::string & record_id) const
{
	if (is_type_excluded(rec_type))
		return true;

	for (const auto & regex : m_id_regexes)
	{
		if (std::regex_search(record_id, regex))
			return true;
	}

	return false;
}

std::set<std::string> merge_exclusions_t::ignored_sub_records() const
{
	std::set<std::string> result;
	for (const auto & rule : m_rules)
	{
		if (rule.kind == exclude_kind_t::sub_record)
			result.insert(rule.target);
	}

	return result;
}
