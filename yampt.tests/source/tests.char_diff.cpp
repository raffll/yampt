#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <utility/char_diff.hpp>
#include <rapidcheck.h>
#include <string>

TEST_CASE("compute_char_diff, reconstruction property", "[u]")
{
	rc::prop(
	    "concatenating unchanged+inserted reproduces new_text and unchanged+deleted reproduces old_text",
	    []()
	{
		const auto old_text = *rc::gen::arbitrary<std::string>();
		const auto new_text = *rc::gen::arbitrary<std::string>();

		const auto segments = compute_char_diff(old_text, new_text);

		std::string reconstructed_new;
		std::string reconstructed_old;

		for (const auto & segment : segments)
		{
			if (segment.operation == diff_op_t::unchanged || segment.operation == diff_op_t::inserted)
				reconstructed_new += segment.text;

			if (segment.operation == diff_op_t::unchanged || segment.operation == diff_op_t::deleted)
				reconstructed_old += segment.text;
		}

		RC_ASSERT(reconstructed_new == new_text);
		RC_ASSERT(reconstructed_old == old_text);
	});
}

TEST_CASE("compute_char_diff, empty strings", "[u]")
{
	SECTION("both empty")
	{
		const auto segments = compute_char_diff("", "");
		REQUIRE(segments.size() == 1);
		REQUIRE(segments[0].operation == diff_op_t::unchanged);
		REQUIRE(segments[0].text.empty());
	}

	SECTION("old empty")
	{
		const auto segments = compute_char_diff("", "hello");
		REQUIRE(segments.size() == 1);
		REQUIRE(segments[0].operation == diff_op_t::inserted);
		REQUIRE(segments[0].text == "hello");
	}

	SECTION("new empty")
	{
		const auto segments = compute_char_diff("hello", "");
		REQUIRE(segments.size() == 1);
		REQUIRE(segments[0].operation == diff_op_t::deleted);
		REQUIRE(segments[0].text == "hello");
	}
}

TEST_CASE("compute_char_diff, identical strings", "[u]")
{
	const auto segments = compute_char_diff("Balmora", "Balmora");
	REQUIRE(segments.size() == 1);
	REQUIRE(segments[0].operation == diff_op_t::unchanged);
	REQUIRE(segments[0].text == "Balmora");
}

TEST_CASE("compute_char_diff, completely different strings", "[u]")
{
	const auto segments = compute_char_diff("abc", "xyz");

	std::string reconstructed_new;
	std::string reconstructed_old;

	for (const auto & segment : segments)
	{
		if (segment.operation == diff_op_t::unchanged || segment.operation == diff_op_t::inserted)
			reconstructed_new += segment.text;

		if (segment.operation == diff_op_t::unchanged || segment.operation == diff_op_t::deleted)
			reconstructed_old += segment.text;
	}

	REQUIRE(reconstructed_new == "xyz");
	REQUIRE(reconstructed_old == "abc");
}

namespace {

struct highlight_range_t
{
	int start;
	int end;
};

std::vector<highlight_range_t> compute_changed_highlights(
    const std::string & current_text,
    const std::string & details_text)
{
	std::vector<highlight_range_t> ranges;
	const auto segments = compute_char_diff(current_text, details_text);

	int position = 0;
	for (const auto & segment : segments)
	{
		const int length = static_cast<int>(segment.text.size());

		if (segment.operation == diff_op_t::inserted)
		{
			ranges.push_back({ position, position + length });
			position += length;
		}
		else if (segment.operation == diff_op_t::deleted)
		{
			const int mark_pos = position < static_cast<int>(details_text.size()) ? position : position - 1;
			if (mark_pos >= 0 && mark_pos < static_cast<int>(details_text.size()))
				ranges.push_back({ mark_pos, mark_pos + 1 });
		}
		else if (segment.operation == diff_op_t::unchanged)
		{
			position += length;
		}
	}
	return ranges;
}

} // namespace

TEST_CASE("compute_changed_highlights, identical texts produce no highlights", "[u]")
{
	const auto ranges = compute_changed_highlights("Balmora", "Balmora");
	REQUIRE(ranges.empty());
}

TEST_CASE("compute_changed_highlights, single character change", "[u]")
{
	const auto ranges = compute_changed_highlights("cat", "car");
	REQUIRE(!ranges.empty());
	bool highlights_position_2 = false;
	for (const auto & r : ranges)
	{
		if (r.start <= 2 && r.end >= 3)
			highlights_position_2 = true;
	}
	REQUIRE(highlights_position_2);
}

TEST_CASE("compute_changed_highlights, details text is shorter", "[u]")
{
	const auto ranges = compute_changed_highlights("hello", "heo");
	REQUIRE(!ranges.empty());
	for (const auto & r : ranges)
	{
		REQUIRE(r.start >= 0);
		REQUIRE(r.end <= 3);
	}
}

TEST_CASE("compute_changed_highlights, details text is longer", "[u]")
{
	const auto ranges = compute_changed_highlights("heo", "hello");
	REQUIRE(!ranges.empty());
	bool found_insertion = false;
	for (const auto & r : ranges)
	{
		REQUIRE(r.start >= 0);
		REQUIRE(r.end <= 5);
		if (r.end - r.start > 0)
			found_insertion = true;
	}
	REQUIRE(found_insertion);
}

TEST_CASE("compute_changed_highlights, deletion at end highlights last char", "[u]")
{
	const auto ranges = compute_changed_highlights("abcde", "abc");
	bool has_highlight_near_end = false;
	for (const auto & r : ranges)
	{
		if (r.start >= 2)
			has_highlight_near_end = true;
	}
	REQUIRE(has_highlight_near_end);
}

TEST_CASE("compute_changed_highlights, deletion at start highlights first char", "[u]")
{
	const auto ranges = compute_changed_highlights("XXabc", "abc");
	bool has_highlight_at_start = false;
	for (const auto & r : ranges)
	{
		if (r.start == 0)
			has_highlight_at_start = true;
	}
	REQUIRE(has_highlight_at_start);
}

TEST_CASE("compute_changed_highlights, completely different texts", "[u]")
{
	const auto ranges = compute_changed_highlights("abc", "xyz");
	REQUIRE(!ranges.empty());
	int total_highlighted = 0;
	for (const auto & r : ranges)
		total_highlighted += r.end - r.start;
	REQUIRE(total_highlighted >= 3);
}

TEST_CASE("compute_changed_highlights, prefix differs", "[u]")
{
	const auto ranges = compute_changed_highlights("Hello world", "Goodbye world");
	REQUIRE(!ranges.empty());
	bool highlights_start = false;
	for (const auto & r : ranges)
	{
		if (r.start == 0)
			highlights_start = true;
	}
	REQUIRE(highlights_start);
}

TEST_CASE("compute_changed_highlights, suffix differs", "[u]")
{
	const auto ranges = compute_changed_highlights("Hello world", "Hello earth");
	REQUIRE(!ranges.empty());
	bool highlights_end_region = false;
	for (const auto & r : ranges)
	{
		if (r.end > 6)
			highlights_end_region = true;
	}
	REQUIRE(highlights_end_region);
}

TEST_CASE("compute_changed_highlights, middle insertion", "[u]")
{
	const auto ranges = compute_changed_highlights("ac", "abc");
	REQUIRE(!ranges.empty());
	bool highlights_middle = false;
	for (const auto & r : ranges)
	{
		if (r.start == 1)
			highlights_middle = true;
	}
	REQUIRE(highlights_middle);
}

TEST_CASE("compute_changed_highlights, middle deletion", "[u]")
{
	const auto ranges = compute_changed_highlights("abc", "ac");
	REQUIRE(!ranges.empty());
	bool highlights_deletion_point = false;
	for (const auto & r : ranges)
	{
		if (r.start <= 1 && r.end >= 1)
			highlights_deletion_point = true;
	}
	REQUIRE(highlights_deletion_point);
}

TEST_CASE("compute_changed_highlights, empty current text", "[u]")
{
	const auto ranges = compute_changed_highlights("", "hello");
	REQUIRE(!ranges.empty());
	int total = 0;
	for (const auto & r : ranges)
		total += r.end - r.start;
	REQUIRE(total == 5);
}

TEST_CASE("compute_changed_highlights, empty details text", "[u]")
{
	const auto ranges = compute_changed_highlights("hello", "");
	REQUIRE(ranges.empty());
}

TEST_CASE("compute_changed_highlights, ranges within bounds", "[u]")
{
	const auto ranges = compute_changed_highlights("The quick brown fox", "A slow red dog");
	for (const auto & r : ranges)
	{
		REQUIRE(r.start >= 0);
		REQUIRE(r.end <= 14);
		REQUIRE(r.start < r.end);
	}
}

TEST_CASE("compute_changed_highlights, single char details with deletion", "[u]")
{
	const auto ranges = compute_changed_highlights("abcdef", "x");
	REQUIRE(!ranges.empty());
	for (const auto & r : ranges)
	{
		REQUIRE(r.start >= 0);
		REQUIRE(r.end <= 1);
	}
}

TEST_CASE("compute_changed_highlights, ranges never exceed details length", "[u]")
{
	rc::prop(
	    "all highlight ranges are within [0, details.size()]",
	    []()
	{
		const auto current = *rc::gen::arbitrary<std::string>();
		const auto details = *rc::gen::nonEmpty<std::string>();

		const auto ranges = compute_changed_highlights(current, details);
		const int details_len = static_cast<int>(details.size());

		for (const auto & r : ranges)
		{
			RC_ASSERT(r.start >= 0);
			RC_ASSERT(r.end <= details_len);
			RC_ASSERT(r.start < r.end);
		}
	});
}

TEST_CASE("compute_changed_highlights, no highlights when texts identical", "[u]")
{
	rc::prop(
	    "identical texts never produce highlights",
	    []()
	{
		const auto text = *rc::gen::arbitrary<std::string>();
		const auto ranges = compute_changed_highlights(text, text);
		RC_ASSERT(ranges.empty());
	});
}
