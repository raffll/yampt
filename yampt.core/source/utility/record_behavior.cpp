#include "record_behavior.hpp"
#include "../decoder/sub_record_schema.hpp"
#include <cstring>
#include <map>
#include <set>

using enum sub_rule_flag_t;

static constexpr sub_record_rule_t cell_wildcard = { "*", 0, skip_non_existent };

static constexpr sub_record_rule_t cell_sub_rules[] = {
	{ "NAM0", 0, skip_emit },
	{ "FRMR", 0, merge_boundary },
};

static constexpr field_pair_rule_t crea_npdt_attack_pairs[] = {
	{ 68, 72, 4 },
	{ 76, 80, 4 },
	{ 84, 88, 4 },
};

static constexpr paired_merge_rule_t crea_paired_rules[] = {
	{ "NPDT", 96, crea_npdt_attack_pairs, 3 },
};

static constexpr field_pair_rule_t weap_wpdt_damage_pairs[] = {
	{ 22, 23, 1 },
	{ 24, 25, 1 },
	{ 26, 27, 1 },
};

static constexpr paired_merge_rule_t weap_paired_rules[] = {
	{ "WPDT", 32, weap_wpdt_damage_pairs, 3 },
};

static constexpr sub_record_rule_t npc_sub_rules[] = {
	{ "NPDT", 52, skip_if_size_differs | element_wise_merge },
	{ "NPDT", 12, skip_if_size_differs | element_wise_merge },
	{ "AIDT", 12, element_wise_merge },
	{ "FLAG", 4, element_wise_merge },
};

static constexpr sub_record_rule_t crea_sub_rules[] = {
	{ "NPDT", 96, element_wise_merge },
	{ "AI_W", 14, element_wise_merge },
	{ "AIDT", 12, element_wise_merge },
	{ "FLAG", 4, element_wise_merge },
};

static constexpr sub_record_rule_t cont_sub_rules[] = {
	{ "FLAG", 4, element_wise_merge },
};

static constexpr sub_record_rule_t levi_sub_rules[] = {
	{ "DATA", 4, element_wise_merge },
};

static constexpr sub_record_rule_t weap_sub_rules[] = {
	{ "WPDT", 32, element_wise_merge },
};

static constexpr sub_record_rule_t armo_sub_rules[] = {
	{ "AODT", 24, element_wise_merge },
};

static constexpr sub_record_rule_t fact_sub_rules[] = {
	{ "FADT", 240, element_wise_merge },
};

static constexpr const char * fact_reaction_members[] = { "INTV" };

static constexpr const char * armor_part_sub_types[] = { "BNAM", "CNAM" };

static constexpr keyed_list_spec_t npco_keyed_list_spec[] = {
	{ .sub_type = "NPCO",
	  .key_offset = npco_layout::item_id_offset,
	  .key_length = npco_layout::item_id_length,
	  .minimum_record_size = npco_layout::record_size,
	  .key_source = key_source_t::content_slice },
};

static constexpr keyed_list_spec_t npcs_keyed_list_spec[] = {
	{ .sub_type = "NPCS",
	  .key_offset = 0,
	  .key_length = 0,
	  .minimum_record_size = 0,
	  .key_source = key_source_t::whole_content },
};

static constexpr keyed_list_spec_t inventory_and_spell_keyed_list_specs[] = {
	{ .sub_type = "NPCO",
	  .key_offset = npco_layout::item_id_offset,
	  .key_length = npco_layout::item_id_length,
	  .minimum_record_size = npco_layout::record_size,
	  .key_source = key_source_t::content_slice },
	{ .sub_type = "NPCS",
	  .key_offset = 0,
	  .key_length = 0,
	  .minimum_record_size = 0,
	  .key_source = key_source_t::whole_content },
};

static constexpr keyed_list_spec_t enam_keyed_list_spec[] = {
	{ .sub_type = "ENAM",
	  .key_offset = 0,
	  .key_length = enam_layout::identity_prefix_length,
	  .minimum_record_size = enam_layout::slot_size,
	  .key_source = key_source_t::identity_prefix },
};

static constexpr keyed_list_spec_t fact_reaction_keyed_list_spec[] = {
	{ .sub_type = "ANAM",
	  .key_offset = 0,
	  .key_length = 0,
	  .minimum_record_size = 0,
	  .key_source = key_source_t::paired_name,
	  .member_sub_types = fact_reaction_members,
	  .member_sub_type_count = 1 },
};

static constexpr keyed_list_spec_t armor_part_keyed_list_spec[] = {
	{ .sub_type = "INDX",
	  .key_offset = 0,
	  .key_length = 0,
	  .minimum_record_size = 0,
	  .key_source = key_source_t::index_value,
	  .member_sub_types = armor_part_sub_types,
	  .member_sub_type_count = 2 },
};

static constexpr sub_record_rule_t race_sub_rules[] = {
	{ "RADT", 140, element_wise_merge },
};

static constexpr sub_record_rule_t generic_sub_rules[] = {
	{ "AIDT", 12, element_wise_merge },
};

static constexpr record_behavior_t behavior_table[] = {
	{ .record_type = "CELL",
	  .decode_mode = decode_mode_t::cell,
	  .copy_strategy = copy_strategy_t::header_and_selected_group,
	  .sub_rules = cell_sub_rules,
	  .sub_rule_count = 2,
	  .wildcard_rule = &cell_wildcard },
	{ .record_type = "LEVI",
	  .decode_mode = decode_mode_t::leveled,
	  .sub_rules = levi_sub_rules,
	  .sub_rule_count = 1,
	  .leveled_item_sub_type = "INAM",
	  .leveled_level_sub_type = "INTV" },
	{ .record_type = "LEVC",
	  .decode_mode = decode_mode_t::leveled,
	  .sub_rules = levi_sub_rules,
	  .sub_rule_count = 1,
	  .leveled_item_sub_type = "CNAM",
	  .leveled_level_sub_type = "INTV" },
	{ .record_type = "FACT",
	  .decode_mode = decode_mode_t::faction,
	  .sub_rules = fact_sub_rules,
	  .sub_rule_count = 1,
	  .keyed_list_specs = fact_reaction_keyed_list_spec,
	  .keyed_list_spec_count = 1 },
	{ .record_type = "CONT",
	  .decode_mode = decode_mode_t::container,
	  .sub_rules = cont_sub_rules,
	  .sub_rule_count = 1,
	  .keyed_list_specs = npco_keyed_list_spec,
	  .keyed_list_spec_count = 1 },
	{ .record_type = "BSGN", .decode_mode = decode_mode_t::container },
	{ .record_type = "RACE",
	  .decode_mode = decode_mode_t::container,
	  .sub_rules = race_sub_rules,
	  .sub_rule_count = 1,
	  .keyed_list_specs = npcs_keyed_list_spec,
	  .keyed_list_spec_count = 1 },
	{ .record_type = "NPC_",
	  .decode_mode = decode_mode_t::container,
	  .sub_rules = npc_sub_rules,
	  .sub_rule_count = 4,
	  .keyed_list_specs = inventory_and_spell_keyed_list_specs,
	  .keyed_list_spec_count = 2 },
	{ .record_type = "CREA",
	  .decode_mode = decode_mode_t::container,
	  .sub_rules = crea_sub_rules,
	  .sub_rule_count = 4,
	  .paired_rules = crea_paired_rules,
	  .paired_rule_count = 1,
	  .keyed_list_specs = inventory_and_spell_keyed_list_specs,
	  .keyed_list_spec_count = 2 },
	{ .record_type = "WEAP",
	  .sub_rules = weap_sub_rules,
	  .sub_rule_count = 1,
	  .paired_rules = weap_paired_rules,
	  .paired_rule_count = 1 },
	{ .record_type = "ARMO",
	  .decode_mode = decode_mode_t::armor,
	  .sub_rules = armo_sub_rules,
	  .sub_rule_count = 1,
	  .keyed_list_specs = armor_part_keyed_list_spec,
	  .keyed_list_spec_count = 1,
	  .armor_part_sub_types = armor_part_sub_types,
	  .armor_part_sub_type_count = 2 },
	{ .record_type = "CLOT",
	  .decode_mode = decode_mode_t::armor,
	  .keyed_list_specs = armor_part_keyed_list_spec,
	  .keyed_list_spec_count = 1,
	  .armor_part_sub_types = armor_part_sub_types,
	  .armor_part_sub_type_count = 2 },
	{ .record_type = "SCPT", .merge = merge_case_t::none, .merge_excluded = true },
	{ .record_type = "ENCH",
	  .enam_effect_list = true,
	  .keyed_list_specs = enam_keyed_list_spec,
	  .keyed_list_spec_count = 1 },
	{ .record_type = "SPEL",
	  .enam_effect_list = true,
	  .keyed_list_specs = enam_keyed_list_spec,
	  .keyed_list_spec_count = 1 },
	{ .record_type = "ALCH",
	  .enam_effect_list = true,
	  .keyed_list_specs = enam_keyed_list_spec,
	  .keyed_list_spec_count = 1 },
	{ .record_type = "INGR",
	  .enam_effect_list = true,
	  .keyed_list_specs = enam_keyed_list_spec,
	  .keyed_list_spec_count = 1 },
	{ .record_type = "LAND",
	  .merge = merge_case_t::none,
	  .merge_excluded = true,
	  .read_only_reason = read_only_reason_t::landscape_data,
	  .allows_copy = false,
	  .allows_lock = false },
	{ .record_type = "PGRD", .merge = merge_case_t::none, .merge_excluded = true },
	{ .record_type = "REGN", .merge = merge_case_t::none, .merge_excluded = true },
	{ .record_type = "DIAL",
	  .merge = merge_case_t::none,
	  .decode_mode = decode_mode_t::dial,
	  .merge_excluded = true },
	{ .record_type = "INFO",
	  .merge = merge_case_t::none,
	  .decode_mode = decode_mode_t::info,
	  .merge_excluded = true,
	  .record_id_sub_type = "INAM" },
};

static constexpr record_behavior_t generic_behavior = {
	.record_type = "*", .sub_rules = generic_sub_rules, .sub_rule_count = 1
};

const record_behavior_t * find_record_behavior(const std::string & record_type)
{
	for (const auto & entry : behavior_table)
	{
		if (record_type == entry.record_type)
			return &entry;
	}

	return &generic_behavior;
}

const keyed_list_spec_t * keyed_list_specs_for(const std::string & record_type, size_t & spec_count)
{
	const auto * behavior = find_record_behavior(record_type);
	spec_count = behavior->keyed_list_spec_count;
	return behavior->keyed_list_specs;
}

bool is_armor_part_sub_type(const std::string & record_type, const std::string & sub_type)
{
	const auto * behavior = find_record_behavior(record_type);
	for (size_t i = 0; i < behavior->armor_part_sub_type_count; ++i)
	{
		if (sub_type == behavior->armor_part_sub_types[i])
			return true;
	}

	return false;
}

bool is_enam_effect_list(const std::string & record_type)
{
	return find_record_behavior(record_type)->enam_effect_list;
}

bool is_merge_excluded(const std::string & record_type)
{
	return find_record_behavior(record_type)->merge_excluded;
}

merge_case_t merge_case_for(const std::string & record_type)
{
	return find_record_behavior(record_type)->merge;
}

decode_mode_t decode_mode_for(const std::string & record_type)
{
	return find_record_behavior(record_type)->decode_mode;
}

const char * leveled_item_sub_type_for(const std::string & record_type)
{
	return find_record_behavior(record_type)->leveled_item_sub_type;
}

const char * leveled_level_sub_type_for(const std::string & record_type)
{
	return find_record_behavior(record_type)->leveled_level_sub_type;
}

const char * record_id_sub_type_for(const std::string & record_type)
{
	return find_record_behavior(record_type)->record_id_sub_type;
}

read_only_reason_t read_only_reason_for(const std::string & record_type)
{
	return find_record_behavior(record_type)->read_only_reason;
}

bool record_allows_copy(const std::string & record_type)
{
	return find_record_behavior(record_type)->allows_copy;
}

bool record_allows_lock(const std::string & record_type)
{
	return find_record_behavior(record_type)->allows_lock;
}

bool record_allows_exclude(const std::string & record_type)
{
	return find_record_behavior(record_type)->allows_exclude;
}

const sub_record_rule_t * find_sub_record_rule(
    const record_behavior_t * behavior,
    const std::string & sub_type,
    size_t data_size)
{
	if (!behavior)
		return nullptr;

	for (size_t i = 0; i < behavior->sub_rule_count; ++i)
	{
		const auto & rule = behavior->sub_rules[i];
		if (sub_type != rule.sub_type)
			continue;

		if (rule.expected_size != 0 && data_size != 0 && rule.expected_size != data_size)
			continue;

		return &rule;
	}

	return behavior->wildcard_rule;
}

field_pair_role_t find_field_pair_role(
    const std::string & record_type,
    const std::string & sub_type,
    size_t field_offset)
{
	const auto * behavior = find_record_behavior(record_type);
	if (!behavior)
		return field_pair_role_t::none;

	for (size_t rule_idx = 0; rule_idx < behavior->paired_rule_count; ++rule_idx)
	{
		const auto & rule = behavior->paired_rules[rule_idx];
		if (sub_type != rule.sub_type)
			continue;

		for (size_t pair_idx = 0; pair_idx < rule.pair_count; ++pair_idx)
		{
			const auto & pair = rule.pairs[pair_idx];
			if (field_offset == pair.min_offset)
				return field_pair_role_t::min_bound;

			if (field_offset == pair.max_offset)
				return field_pair_role_t::max_bound;
		}
	}

	return field_pair_role_t::none;
}

bool is_repeatable_sub_record(const std::string & record_type, const std::string & sub_type)
{
	for (const auto & entry : record_composition(record_type))
	{
		if (sub_type == entry.sub_type)
			return entry.kind == sub_record_kind_t::repeatable;
	}

	return false;
}
