#include "highlight_coordinator.hpp"
#include "../editor/glossary.hpp"
#include <utility/string_utils.hpp>
#include <algorithm>

struct highlight_candidate_t
{
	int start;
	int length;
	highlight_kind_t kind;
};

static int kind_priority(highlight_kind_t kind)
{
	switch (kind)
	{
	case highlight_kind_t::hyperlink:
		return 0;
	case highlight_kind_t::inflection:
		return 1;
	case highlight_kind_t::glossary:
		return 2;
	}

	return 3;
}

static highlight_kind_t kind_for_annotation(const annotation_t & annotation)
{
	if (annotation.kind == annotation_t::dial_topic)
		return highlight_kind_t::hyperlink;

	if (annotation.kind == annotation_t::inflection_form)
		return highlight_kind_t::inflection;

	return highlight_kind_t::glossary;
}

std::vector<annotation_t> highlight_coordinator_t::combine_translation_annotations(
    const std::vector<annotation_t> & glossary_annotations,
    const std::vector<annotation_t> & inflection_annotations)
{
	std::vector<annotation_t> combined = glossary_annotations;
	combined.insert(combined.end(), inflection_annotations.begin(), inflection_annotations.end());

	return combined;
}

std::vector<highlight_position_t> highlight_coordinator_t::find_annotation_highlights(
    const std::string & text_lower,
    const highlight_request_t & request)
{
	std::vector<highlight_candidate_t> candidates;

	for (const auto & annotation : *request.annotations)
	{
		const auto kind = kind_for_annotation(annotation);
		const auto & raw =
		    kind == highlight_kind_t::inflection ? annotation.old_text
		                                         : (request.use_old_text ? annotation.old_text : annotation.new_text);
		if (raw.empty())
			continue;

		const auto term = string_utils::to_lower_utf8(raw);
		const auto term_length = static_cast<int>(term.length());

		size_t pos = 0;
		while ((pos = text_lower.find(term, pos)) != std::string::npos)
		{
			candidates.push_back({ static_cast<int>(pos), term_length, kind });
			pos += static_cast<size_t>(term_length);
		}
	}

	if (request.sort_policy == highlight_sort_policy_t::hyperlink_first)
	{
		std::sort(
		    candidates.begin(),
		    candidates.end(),
		    [](const highlight_candidate_t & first, const highlight_candidate_t & second)
		{
			if (kind_priority(first.kind) != kind_priority(second.kind))
				return kind_priority(first.kind) < kind_priority(second.kind);

			if (first.length != second.length)
				return first.length > second.length;

			return first.start < second.start;
		});
	}
	else
	{
		std::sort(
		    candidates.begin(),
		    candidates.end(),
		    [](const highlight_candidate_t & first, const highlight_candidate_t & second)
		{
			if (first.length != second.length)
				return first.length > second.length;

			if (kind_priority(first.kind) != kind_priority(second.kind))
				return kind_priority(first.kind) < kind_priority(second.kind);

			return first.start < second.start;
		});
	}

	std::vector<bool> covered(text_lower.length(), false);
	std::vector<highlight_position_t> results;

	for (const auto & candidate : candidates)
	{
		bool overlap = false;
		for (int i = candidate.start; i < candidate.start + candidate.length; ++i)
		{
			if (covered[static_cast<size_t>(i)])
			{
				overlap = true;
				break;
			}
		}

		if (overlap)
			continue;

		for (int i = candidate.start; i < candidate.start + candidate.length; ++i)
			covered[static_cast<size_t>(i)] = true;

		results.push_back({ candidate.start, candidate.length, candidate.kind });
	}

	return results;
}

std::vector<highlight_position_t> highlight_coordinator_t::find_grammar_highlights(
    const std::string & text,
    const std::vector<std::pair<int, int>> & misspelled_ranges)
{
	std::vector<highlight_position_t> results;
	const auto text_length = static_cast<int>(text.length());

	for (const auto & [range_start, range_end] : misspelled_ranges)
	{
		if (range_start < 0 || range_end <= range_start || range_end > text_length)
			continue;

		results.push_back({ range_start, range_end - range_start, highlight_kind_t::glossary });
	}

	return results;
}
