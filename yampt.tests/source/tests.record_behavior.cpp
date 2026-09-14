#include <catch2/catch_all.hpp>
#include <utility/record_behavior.hpp>

TEST_CASE("record_behavior::is_repeatable_sub_record, effect list is repeatable", "[u]")
{
	REQUIRE(is_repeatable_sub_record("SPEL", "ENAM"));
	REQUIRE(is_repeatable_sub_record("ENCH", "ENAM"));
	REQUIRE(is_repeatable_sub_record("ALCH", "ENAM"));
}

TEST_CASE("record_behavior::is_repeatable_sub_record, inventory and spell lists are repeatable", "[u]")
{
	REQUIRE(is_repeatable_sub_record("NPC_", "NPCO"));
	REQUIRE(is_repeatable_sub_record("NPC_", "NPCS"));
	REQUIRE(is_repeatable_sub_record("CONT", "NPCO"));
	REQUIRE(is_repeatable_sub_record("RACE", "NPCS"));
	REQUIRE(is_repeatable_sub_record("FACT", "RNAM"));
}

TEST_CASE("record_behavior::is_repeatable_sub_record, single-value sub-records are not repeatable", "[u]")
{
	REQUIRE_FALSE(is_repeatable_sub_record("SPEL", "NAME"));
	REQUIRE_FALSE(is_repeatable_sub_record("SPEL", "FNAM"));
	REQUIRE_FALSE(is_repeatable_sub_record("NPC_", "NPDT"));
	REQUIRE_FALSE(is_repeatable_sub_record("ARMO", "AODT"));
}

TEST_CASE("record_behavior::is_repeatable_sub_record, unknown record type is not repeatable", "[u]")
{
	REQUIRE_FALSE(is_repeatable_sub_record("XXXX", "ENAM"));
	REQUIRE_FALSE(is_repeatable_sub_record("INFO", "NAME"));
}
