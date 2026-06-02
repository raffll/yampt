#pragma once

#include "../yampt/tools.hpp"
#include <set>
#include <string>
#include <vector>

class editor_state_t;

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
	void find_all(const editor_state_t & state, const std::set<tools_t::rec_type_t> & type_filter);
	const search_match_t * current_match() const;
	void next_match();
	void prev_match();
	void replace_current(editor_state_t & state, const std::string & replacement);
	size_t replace_all(editor_state_t & state, const std::string & replacement);

	const std::vector<search_match_t> & get_matches() const;
	size_t current_index() const;

private:
	std::string query_;
	bool case_sensitive_ = false;
	std::vector<search_match_t> matches_;
	size_t current_ = 0;
};
