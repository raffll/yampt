#include <catch2/catch_all.hpp>
#include <view/type_ahead.hpp>

TEST_CASE("type_ahead::next_matching_row, empty list returns -1", "[u]")
{
	const std::vector<std::string> filenames;
	REQUIRE(type_ahead::next_matching_row(filenames, "a", -1) == -1);
}

TEST_CASE("type_ahead::next_matching_row, single match returns its row", "[u]")
{
	const std::vector<std::string> filenames = { "alpha" };
	REQUIRE(type_ahead::next_matching_row(filenames, "a", -1) == 0);
}

TEST_CASE("type_ahead::next_matching_row, no match returns -1", "[u]")
{
	const std::vector<std::string> filenames = { "alpha", "beta" };
	REQUIRE(type_ahead::next_matching_row(filenames, "z", -1) == -1);
}

TEST_CASE("type_ahead::next_matching_row, first match with start -1", "[u]")
{
	const std::vector<std::string> filenames = { "beta", "bravo", "alpha" };
	REQUIRE(type_ahead::next_matching_row(filenames, "b", -1) == 0);
}

TEST_CASE("type_ahead::next_matching_row, wraps to first match", "[u]")
{
	const std::vector<std::string> filenames = { "beta", "alpha", "bravo" };
	REQUIRE(type_ahead::next_matching_row(filenames, "b", 2) == 0);
}

TEST_CASE("type_ahead::next_matching_row, start just before a match returns next match", "[u]")
{
	const std::vector<std::string> filenames = { "alpha", "beta", "bravo" };
	REQUIRE(type_ahead::next_matching_row(filenames, "b", 0) == 1);
}

TEST_CASE("type_ahead::next_matching_row, case insensitive match", "[u]")
{
	const std::vector<std::string> filenames = { "Alpha", "BETA" };
	REQUIRE(type_ahead::next_matching_row(filenames, "b", -1) == 1);
}

TEST_CASE("type_ahead::next_matching_row, prefix longer than any filename returns -1", "[u]")
{
	const std::vector<std::string> filenames = { "ab", "cd" };
	REQUIRE(type_ahead::next_matching_row(filenames, "abcd", -1) == -1);
}
