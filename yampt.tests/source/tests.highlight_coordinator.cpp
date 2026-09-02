#include <catch2/catch_all.hpp>
#include <editor/glossary.hpp>
#include <highlighter/highlight_coordinator.hpp>
#include <rapidcheck/catch.h>
#include <rapidcheck.h>
#include <algorithm>

TEST_CASE("highlight_coordinator_t::find_annotation_highlights, position bounds", "[pbt]")
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

TEST_CASE("highlight_coordinator_t::find_annotation_highlights, non-ASCII annotation matched", "[u]")
{
	std::vector<annotation_t> annotations;
	annotation_t entry;
	entry.start = 0;
	entry.end = 0;
	entry.kind = annotation_t::glossary_term;
	entry.old_text = "\xc3\x96" "dsee";
	entry.new_text = "lake";
	entry.source = "test.json";
	annotations.push_back(entry);

	const std::string text_lower = "die \xc3\xb6" "dsee ist kalt";

	highlight_request_t request;
	request.annotations = &annotations;
	request.use_old_text = true;
	request.sort_policy = highlight_sort_policy_t::length_first;

	const auto results = highlight_coordinator_t::find_annotation_highlights(text_lower, request);

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].start == 4);
	REQUIRE(results[0].length == 6);
}

TEST_CASE("highlight_coordinator_t::find_annotation_highlights, uppercase annotation matched in lowercase text", "[u]")
{
	std::vector<annotation_t> annotations;
	annotation_t entry;
	entry.start = 0;
	entry.end = 0;
	entry.kind = annotation_t::dial_topic;
	entry.old_text = "B\xc4\x84lmora";
	entry.new_text = "b\xc4\x85lmora";
	entry.source = "test.json";
	annotations.push_back(entry);

	const std::string text_lower = "witaj w b\xc4\x85lmora";

	highlight_request_t request;
	request.annotations = &annotations;
	request.use_old_text = true;
	request.sort_policy = highlight_sort_policy_t::length_first;

	const auto results = highlight_coordinator_t::find_annotation_highlights(text_lower, request);

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].start == 8);
	REQUIRE(results[0].length == 8);
	REQUIRE(results[0].kind == highlight_kind_t::hyperlink);
}

TEST_CASE("highlight_coordinator_t::combine_translation_annotations, inflected form highlighted in translation", "[u]")
{
	std::vector<annotation_t> glossary_annotations;
	annotation_t term;
	term.start = 0;
	term.end = 0;
	term.kind = annotation_t::glossary_term;
	term.old_text = "services";
	term.new_text = "us\xc5\x82ugi";
	term.source = "test.json";
	glossary_annotations.push_back(term);

	std::vector<annotation_t> inflection_annotations;
	annotation_t inflected;
	inflected.start = 0;
	inflected.end = 0;
	inflected.kind = annotation_t::inflection_form;
	inflected.old_text = "us\xc5\x82ugach";
	inflected.new_text = "us\xc5\x82ug";
	inflected.source = "PL_BASE.top";
	inflection_annotations.push_back(inflected);

	const auto combined =
	    highlight_coordinator_t::combine_translation_annotations(glossary_annotations, inflection_annotations);

	const std::string text_lower = "je\xc5\x9bli interesuj\xc4\x85 ci\xc4\x99 moje us\xc5\x82ugach";

	highlight_request_t request;
	request.annotations = &combined;
	request.use_old_text = false;
	request.sort_policy = highlight_sort_policy_t::hyperlink_first;

	const auto results = highlight_coordinator_t::find_annotation_highlights(text_lower, request);

	const bool inflection_highlighted = std::any_of(
	    results.begin(),
	    results.end(),
	    [](const highlight_position_t & pos) { return pos.kind == highlight_kind_t::inflection; });

	REQUIRE(inflection_highlighted);
}
