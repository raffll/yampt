#include <catch2/catch_test_macros.hpp>

#include <creator/inflection.hpp>

TEST_CASE("inflection_t::word_forms, returns empty when not loaded", "[u]")
{
	inflection_t inflection;
	REQUIRE(inflection.is_loaded() == false);
	const auto & result = inflection.word_forms("test");
	REQUIRE(result.empty());
}

TEST_CASE("inflection_t::phrase_forms, single word returns word_forms", "[u]")
{
	inflection_t inflection;
	const auto & result = inflection.phrase_forms("test");
	REQUIRE(result.empty());
}

TEST_CASE("inflection_t::phrase_forms, multi-word inflects one at a time", "[u]")
{
	inflection_t inflection;
	const auto & result = inflection.phrase_forms("silver sword");
	REQUIRE(result.empty());
}
