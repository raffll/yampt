#include <catch2/catch_all.hpp>
#include <creator/cell_matcher.hpp>
#include <creator/dial_matcher.hpp>
#include <rapidcheck/catch.h>
#include <cstring>
#include <rapidcheck.h>

TEST_CASE("cell_matcher_t::is_interior_cell, short input", "[u][pbt]")
{
	rc::prop(
	    "strings shorter than 4 bytes always return false",
	    []()
	{
		const auto length = *rc::gen::inRange(0, 4);
		auto data_content = *rc::gen::arbitrary<std::string>();
		data_content.resize(length);

		RC_ASSERT(cell_matcher_t::is_interior_cell(data_content) == false);
	});
}

TEST_CASE("cell_matcher_t::make_cell_key_text, collision resistant", "[u][pbt]")
{
	rc::prop(
	    "two different non-empty fingerprints produce different key_text",
	    []()
	{
		auto fp_a = *rc::gen::arbitrary<std::string>();
		auto fp_b = *rc::gen::arbitrary<std::string>();
		RC_PRE(!fp_a.empty());
		RC_PRE(!fp_b.empty());
		RC_PRE(fp_a != fp_b);

		const auto key_a = cell_matcher_t::make_cell_key_text(fp_a);
		const auto key_b = cell_matcher_t::make_cell_key_text(fp_b);

		RC_CLASSIFY(key_a == key_b, "collision");
		RC_CLASSIFY(key_a != key_b, "distinct");
	});
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, coord roundtrip", "[u][pbt]")
{
	rc::prop(
	    "coord key contains the encoded GridX and GridY values",
	    []()
	{
		const auto grid_x = *rc::gen::arbitrary<int32_t>();
		const auto grid_y = *rc::gen::arbitrary<int32_t>();

		std::string data_content(12, '\0');
		std::memcpy(data_content.data() + 4, &grid_x, 4);
		std::memcpy(data_content.data() + 8, &grid_y, 4);

		const auto result = cell_matcher_t::make_exterior_coord_key(data_content);
		const auto expected = "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";
		RC_ASSERT(result == expected);
	});
}

TEST_CASE("dial_matcher_t, fingerprint_index_t resolves correctly", "[u]")
{
	dial_matcher_t::fingerprint_index_t index;
	index["test_inam"].insert(0);
	index["test_inam"].insert(5);
	REQUIRE(index["test_inam"].size() == 2);
	REQUIRE(index["test_inam"].count(0) == 1);
	REQUIRE(index["test_inam"].count(5) == 1);
}
