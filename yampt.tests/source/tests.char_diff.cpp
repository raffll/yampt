#include <catch2/catch_all.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include <utility/char_diff.hpp>
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
