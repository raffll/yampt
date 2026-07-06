#pragma once

#include <string>
#include <vector>

struct annotation_t;

struct highlight_position_t
{
	int start;
	int length;
	bool is_hyperlink;
};

enum class highlight_sort_policy_t
{
	length_first,
	hyperlink_first,
};

struct highlight_request_t
{
	const std::vector<annotation_t> * annotations;
	bool use_old_text;
	highlight_sort_policy_t sort_policy;
};

class highlight_coordinator_t
{
public:
	static std::vector<highlight_position_t> find_annotation_highlights(
	    const std::string & text_lower,
	    const highlight_request_t & request);

	static std::vector<highlight_position_t> find_grammar_highlights(
	    const std::string & text,
	    const std::vector<std::pair<int, int>> & misspelled_ranges);

private:
	static std::string to_lower_ascii(const std::string & input);
};
