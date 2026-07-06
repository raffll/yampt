#include <catch2/catch_all.hpp>
#include <editor/glossary.hpp>
#include <highlighter/highlight_coordinator.hpp>
#include <rapidcheck/catch.h>
#include <rapidcheck.h>

TEST_CASE("highlight_coordinator_t, position bounds", "[pbt]")
{
	rc::prop(
	    "all highlights are bounded by input text",
	    []()
	{
		const auto text = *rc::gen::arbitrary<std::string>();
		const auto annotation_count = *rc::gen::inRange(0, 10);

		std::vector<annotation_t> annotations;
		for (int i = 0; i < annotation_count; ++i)
		{
			annotation_t entry;
			entry.start = 0;
			entry.end = 0;
			entry.kind = *rc::gen::element(annotation_t::dial_topic, annotation_t::glossary_term);
			entry.old_text = *rc::gen::nonEmpty<std::string>();
			entry.new_text = *rc::gen::nonEmpty<std::string>();
			entry.source = "test.json";
			annotations.push_back(std::move(entry));
		}

		std::string text_lower = text;
		for (auto & ch : text_lower)
		{
			if (ch >= 'A' && ch <= 'Z')
				ch = ch + ('a' - 'A');
		}

		highlight_request_t request;
		request.annotations = &annotations;
		request.use_old_text = *rc::gen::arbitrary<bool>();
		request.sort_policy =
		    *rc::gen::element(highlight_sort_policy_t::length_first, highlight_sort_policy_t::hyperlink_first);

		const auto results = highlight_coordinator_t::find_annotation_highlights(text_lower, request);

		for (const auto & pos : results)
		{
			RC_ASSERT(pos.start >= 0);
			RC_ASSERT(pos.length > 0);
			RC_ASSERT(pos.start + pos.length <= static_cast<int>(text.size()));
		}

		const auto misspelled_count = *rc::gen::inRange(0, 8);
		std::vector<std::pair<int, int>> misspelled_ranges;
		const auto text_length = static_cast<int>(text.size());

		for (int i = 0; i < misspelled_count; ++i)
		{
			if (text_length < 2)
				break;

			const auto range_start = *rc::gen::inRange(0, text_length - 1);
			const auto range_end = *rc::gen::inRange(range_start + 1, text_length);
			misspelled_ranges.push_back({ range_start, range_end });
		}

		const auto grammar_results = highlight_coordinator_t::find_grammar_highlights(text, misspelled_ranges);

		for (const auto & pos : grammar_results)
		{
			RC_ASSERT(pos.start >= 0);
			RC_ASSERT(pos.length > 0);
			RC_ASSERT(pos.start + pos.length <= static_cast<int>(text.size()));
		}
	});
}
