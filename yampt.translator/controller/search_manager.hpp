#pragma once

#include "../../yampt/utility/tools.hpp"
#include <set>
#include <string>
#include <vector>

struct search_match_t
{
	tools_t::rec_type_t type;
	size_t record_index;
	size_t char_start;
	size_t char_end;
	bool in_key;
};

class search_manager_t
{
public:
	void set_query(const std::string & text, bool case_sensitive);
	void find_all(const tools_t::dict_t & dict, const std::set<tools_t::rec_type_t> & type_filter);
	const search_match_t * current_match() const;
	void next_match();
	void prev_match();

	const std::vector<search_match_t> & get_matches() const;
	size_t current_index() const;

private:
	std::string query_;
	bool case_sensitive_ = false;
	std::vector<search_match_t> matches_;
	size_t current_ = 0;
};
