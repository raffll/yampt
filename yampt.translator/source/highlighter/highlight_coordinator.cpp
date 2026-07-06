#include "highlight_coordinator.hpp"
#include "../editor/glossary.hpp"
#include <algorithm>

struct highlight_candidate_t
{
	int start;
	int length;
	bool is_hyperlink;
};

std::string highlight_coordinator_t::to_lower_ascii(const std::string & input)
{
	std::string result = input;
	for (auto & ch : result)
	{
		if (ch >= 'A' && ch <= 'Z')
			ch = ch + ('a' - 'A');
	}
	return result;
}

std::vector<highlight_position_t> highlight_coordinator_t::find_annotation_highlights(
    const std::string & text_lower,
    const highlight_request_t & request)
{
	std::vector<highlight_candidate_t> candidates;

	for (const auto & annotation : *request.annotations)
	{
		const auto & raw = request.use_old_text ? annotation.old_text : annotation.new_text;
		if (raw.empty())
			continue;

		const bool is_hyperlink = (annotation.kind == annotation_t::dial_topic);
		const auto term = to_lower_ascii(raw);
		const auto term_length = static_cast<int>(term.length());

		size_t pos = 0;
		while ((pos = text_lower.find(term, pos)) != std::string::npos)
		{
			candidates.push_back({ static_cast<int>(pos), term_length, is_hyperlink });
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
			if (first.is_hyperlink != second.is_hyperlink)
				return first.is_hyperlink;

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

			if (first.is_hyperlink != second.is_hyperlink)
				return first.is_hyperlink;

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

		results.push_back({ candidate.start, candidate.length, candidate.is_hyperlink });
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

		results.push_back({ range_start, range_end - range_start, false });
	}

	return results;
}
