#pragma once

#include "../utility/tools.hpp"
#include <string>
#include <unordered_map>

class text_match_index_t
{
public:
	enum class find_result_t
	{
		found,
		ambiguous,
		not_found
	};

	struct find_outcome_t
	{
		find_result_t result = find_result_t::not_found;
		std::string translation;
		std::string conflicts;
	};

	void build(const tools_t::dict_t & base_dict);
	find_outcome_t find(const std::string & old_text) const;

private:
	std::unordered_map<std::string, const tools_t::record_entry_t *> m_index;
	std::unordered_map<std::string, std::string> m_conflicts;
	std::unordered_map<std::string, std::string> m_first_translation;
};
