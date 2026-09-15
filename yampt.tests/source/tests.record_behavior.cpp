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
	REQUIRE(merge_strategy_for("CELL") == merge_strategy_t::generic);
	REQUIRE(merge_strategy_for("ARMO") == merge_strategy_t::armor_parts);
	REQUIRE(merge_strategy_for("CLOT") == merge_strategy_t::armor_parts);
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

TEST_CASE("record_behavior::is_merge_excluded, baseline excluded record types", "[u]")
{
	REQUIRE(is_merge_excluded("LAND"));
	REQUIRE(is_merge_excluded("PGRD"));
	REQUIRE(is_merge_excluded("REGN"));
	REQUIRE(is_merge_excluded("SCPT"));
	REQUIRE(is_merge_excluded("DIAL"));
	REQUIRE(is_merge_excluded("INFO"));
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

TEST_CASE("record_behavior::leveled_level_sub_type_for, LEVI and LEVC use INTV", "[u]")
{
	REQUIRE(std::string(leveled_level_sub_type_for("LEVI")) == "INTV");
	REQUIRE(std::string(leveled_level_sub_type_for("LEVC")) == "INTV");
	REQUIRE(leveled_level_sub_type_for("WEAP") == nullptr);
	REQUIRE(leveled_level_sub_type_for("XXXX") == nullptr);
}

TEST_CASE("record_behavior::keyed_list_specs_for, inventory and spell lists per record type", "[u]")
{
	size_t npc_count = 0;
	const auto * npc_specs = keyed_list_specs_for("NPC_", npc_count);
	REQUIRE(npc_count == 2);
	REQUIRE(std::string(npc_specs[0].sub_type) == "NPCO");
	REQUIRE(npc_specs[0].key_offset == 4);
	REQUIRE(npc_specs[0].key_length == 32);
	REQUIRE(npc_specs[0].minimum_record_size == 36);
	REQUIRE(std::string(npc_specs[1].sub_type) == "NPCS");
	REQUIRE(npc_specs[1].key_offset == 0);
	REQUIRE(npc_specs[1].key_length == 0);

	size_t crea_count = 0;
	keyed_list_specs_for("CREA", crea_count);
	REQUIRE(crea_count == 2);

	size_t cont_count = 0;
	const auto * cont_specs = keyed_list_specs_for("CONT", cont_count);
	REQUIRE(cont_count == 1);
	REQUIRE(std::string(cont_specs[0].sub_type) == "NPCO");

	size_t race_count = 0;
	const auto * race_specs = keyed_list_specs_for("RACE", race_count);
	REQUIRE(race_count == 1);
	REQUIRE(std::string(race_specs[0].sub_type) == "NPCS");
}

TEST_CASE("record_behavior::keyed_list_specs_for, no specs for unrelated types", "[u]")
{
	size_t weap_count = 0;
	REQUIRE(keyed_list_specs_for("WEAP", weap_count) == nullptr);
	REQUIRE(weap_count == 0);

	size_t unknown_count = 0;
	REQUIRE(keyed_list_specs_for("XXXX", unknown_count) == nullptr);
	REQUIRE(unknown_count == 0);
}

TEST_CASE("record_behavior::reaction_pair_for, FACT pairs ANAM with INTV", "[u]")
{
	const auto * pair = reaction_pair_for("FACT");
	REQUIRE(pair != nullptr);
	REQUIRE(std::string(pair->key_sub_type) == "ANAM");
	REQUIRE(std::string(pair->value_sub_type) == "INTV");

	REQUIRE(reaction_pair_for("NPC_") == nullptr);
	REQUIRE(reaction_pair_for("XXXX") == nullptr);
}

TEST_CASE("record_behavior::is_armor_part_sub_type, ARMO and CLOT part members", "[u]")
{
	REQUIRE(is_armor_part_sub_type("ARMO", "BNAM"));
	REQUIRE(is_armor_part_sub_type("ARMO", "CNAM"));
	REQUIRE(is_armor_part_sub_type("CLOT", "BNAM"));
	REQUIRE(is_armor_part_sub_type("CLOT", "CNAM"));
	REQUIRE_FALSE(is_armor_part_sub_type("ARMO", "INDX"));
	REQUIRE_FALSE(is_armor_part_sub_type("WEAP", "BNAM"));
	REQUIRE_FALSE(is_armor_part_sub_type("XXXX", "CNAM"));
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
