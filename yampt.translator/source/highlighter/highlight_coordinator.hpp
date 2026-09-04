#pragma once

#include <editor/glossary.hpp>
#include <set>
#include <string>
#include <vector>

enum class highlight_kind_t
{
	hyperlink,
	inflection,
	glossary,
};

struct highlight_position_t
{
	int start;
	int length;
	highlight_kind_t kind;
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
	std::set<highlight_kind_t> enabled_kinds;
};

class highlight_coordinator_t
{
public:
	static std::vector<annotation_t> combine_translation_annotations(
	    const std::vector<annotation_t> & glossary_annotations,
	    const std::vector<annotation_t> & inflection_annotations);

	static std::vector<highlight_position_t> find_annotation_highlights(
	    const std::string & text_lower,
	    const highlight_request_t & request);

	static std::vector<highlight_position_t> find_grammar_highlights(
	    const std::string & text,
	    const std::vector<std::pair<int, int>> & misspelled_ranges);
};
