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

TEST_CASE("record_behavior::record_id_sub_type_for, INFO uses INAM others NAME", "[u]")
{
	REQUIRE(std::string(record_id_sub_type_for("INFO")) == "INAM");
	REQUIRE(std::string(record_id_sub_type_for("WEAP")) == "NAME");
	REQUIRE(std::string(record_id_sub_type_for("CELL")) == "NAME");
	REQUIRE(std::string(record_id_sub_type_for("XXXX")) == "NAME");
}

TEST_CASE("record_behavior::read_only_reason_for, LAND is landscape others editable", "[u]")
{
	REQUIRE(read_only_reason_for("LAND") == read_only_reason_t::landscape_data);
	REQUIRE(read_only_reason_for("WEAP") == read_only_reason_t::editable);
	REQUIRE(read_only_reason_for("CELL") == read_only_reason_t::editable);
	REQUIRE(read_only_reason_for("XXXX") == read_only_reason_t::editable);
}

TEST_CASE("record_behavior::record_allows_copy, LAND disallows copy others allow", "[u]")
{
	REQUIRE_FALSE(record_allows_copy("LAND"));
	REQUIRE(record_allows_copy("WEAP"));
	REQUIRE(record_allows_copy("CELL"));
	REQUIRE(record_allows_copy("XXXX"));
}

TEST_CASE("record_behavior::record_allows_lock, LAND disallows lock others allow", "[u]")
{
	REQUIRE_FALSE(record_allows_lock("LAND"));
	REQUIRE(record_allows_lock("WEAP"));
	REQUIRE(record_allows_lock("CELL"));
	REQUIRE(record_allows_lock("XXXX"));
}

TEST_CASE("record_behavior::record_allows_exclude, allowed by default including LAND", "[u]")
{
	REQUIRE(record_allows_exclude("LAND"));
	REQUIRE(record_allows_exclude("WEAP"));
	REQUIRE(record_allows_exclude("XXXX"));
}

TEST_CASE("record_behavior::find_field_pair_role, CREA attack min max pairs", "[u]")
{
	REQUIRE(find_field_pair_role("CREA", "NPDT", 68) == field_pair_role_t::min_bound);
	REQUIRE(find_field_pair_role("CREA", "NPDT", 72) == field_pair_role_t::max_bound);
	REQUIRE(find_field_pair_role("CREA", "NPDT", 60) == field_pair_role_t::none);
}

TEST_CASE("record_behavior::find_field_pair_role, WEAP damage min max pairs", "[u]")
{
	REQUIRE(find_field_pair_role("WEAP", "WPDT", 22) == field_pair_role_t::min_bound);
	REQUIRE(find_field_pair_role("WEAP", "WPDT", 23) == field_pair_role_t::max_bound);
	REQUIRE(find_field_pair_role("WEAP", "WPDT", 24) == field_pair_role_t::min_bound);
	REQUIRE(find_field_pair_role("WEAP", "WPDT", 25) == field_pair_role_t::max_bound);
	REQUIRE(find_field_pair_role("WEAP", "WPDT", 26) == field_pair_role_t::min_bound);
	REQUIRE(find_field_pair_role("WEAP", "WPDT", 27) == field_pair_role_t::max_bound);
}

TEST_CASE("record_behavior::find_field_pair_role, ENAM magnitude min max pairs", "[u]")
{
	REQUIRE(find_field_pair_role("ENCH", "ENAM", 16) == field_pair_role_t::min_bound);
	REQUIRE(find_field_pair_role("ENCH", "ENAM", 20) == field_pair_role_t::max_bound);
	REQUIRE(find_field_pair_role("ALCH", "ENAM", 16) == field_pair_role_t::min_bound);
	REQUIRE(find_field_pair_role("ALCH", "ENAM", 20) == field_pair_role_t::max_bound);
	REQUIRE(find_field_pair_role("SPEL", "ENAM", 16) == field_pair_role_t::min_bound);
	REQUIRE(find_field_pair_role("INGR", "ENAM", 20) == field_pair_role_t::max_bound);
}
