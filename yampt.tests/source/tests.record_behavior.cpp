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

TEST_CASE("record_behavior::merge_strategy_for, dispatch per record type", "[u]")
{
	REQUIRE(merge_strategy_for("CELL") == merge_strategy_t::cell_refs);
	REQUIRE(merge_strategy_for("ARMO") == merge_strategy_t::armor_parts);
	REQUIRE(merge_strategy_for("CLOT") == merge_strategy_t::armor_parts);
	REQUIRE(merge_strategy_for("SCPT") == merge_strategy_t::no_merge);
	REQUIRE(merge_strategy_for("WEAP") == merge_strategy_t::generic);
	REQUIRE(merge_strategy_for("XXXX") == merge_strategy_t::generic);
}

TEST_CASE("record_behavior::is_enam_effect_list, effect-bearing record types", "[u]")
{
	REQUIRE(is_enam_effect_list("ENCH"));
	REQUIRE(is_enam_effect_list("SPEL"));
	REQUIRE(is_enam_effect_list("ALCH"));
	REQUIRE_FALSE(is_enam_effect_list("WEAP"));
	REQUIRE_FALSE(is_enam_effect_list("CELL"));
	REQUIRE_FALSE(is_enam_effect_list("XXXX"));
}

TEST_CASE("record_behavior::is_merge_excluded, LAND is excluded from merge", "[u]")
{
	REQUIRE(is_merge_excluded("LAND"));
	REQUIRE_FALSE(is_merge_excluded("CELL"));
	REQUIRE_FALSE(is_merge_excluded("WEAP"));
	REQUIRE_FALSE(is_merge_excluded("XXXX"));
}

TEST_CASE("record_behavior::decode_mode_for, dispatch per record type", "[u]")
{
	REQUIRE(decode_mode_for("CELL") == decode_mode_t::cell);
	REQUIRE(decode_mode_for("LEVI") == decode_mode_t::leveled);
	REQUIRE(decode_mode_for("LEVC") == decode_mode_t::leveled);
	REQUIRE(decode_mode_for("DIAL") == decode_mode_t::dial);
	REQUIRE(decode_mode_for("INFO") == decode_mode_t::info);
	REQUIRE(decode_mode_for("FACT") == decode_mode_t::faction);
	REQUIRE(decode_mode_for("ARMO") == decode_mode_t::armor);
	REQUIRE(decode_mode_for("WEAP") == decode_mode_t::generic);
	REQUIRE(decode_mode_for("XXXX") == decode_mode_t::generic);
}

TEST_CASE("record_behavior::leveled_item_sub_type_for, LEVI and LEVC item sub-types", "[u]")
{
	REQUIRE(std::string(leveled_item_sub_type_for("LEVI")) == "INAM");
	REQUIRE(std::string(leveled_item_sub_type_for("LEVC")) == "CNAM");
	REQUIRE(leveled_item_sub_type_for("WEAP") == nullptr);
	REQUIRE(leveled_item_sub_type_for("XXXX") == nullptr);
}

TEST_CASE("record_behavior::is_keyed_list_sub_type, FACT reaction sub-types", "[u]")
{
	REQUIRE(is_keyed_list_sub_type("FACT", "ANAM"));
	REQUIRE(is_keyed_list_sub_type("FACT", "INTV"));
	REQUIRE_FALSE(is_keyed_list_sub_type("FACT", "FADT"));
	REQUIRE_FALSE(is_keyed_list_sub_type("NPC_", "ANAM"));
	REQUIRE_FALSE(is_keyed_list_sub_type("XXXX", "ANAM"));
}
