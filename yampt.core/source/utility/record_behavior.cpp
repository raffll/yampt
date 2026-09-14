#include "record_behavior.hpp"
#include "../decoder/sub_record_schema.hpp"
#include <cstring>
#include <map>
#include <set>

using enum sub_rule_flag_t;

static constexpr sub_record_rule_t cell_wildcard = { "*", 0, skip_non_existent };

static constexpr field_pair_rule_t crea_npdt_attack_pairs[] = {
	{ 68, 72, 4 },
	{ 76, 80, 4 },
	{ 84, 88, 4 },
};

static constexpr paired_merge_rule_t crea_paired_rules[] = {
	{ "NPDT", 96, crea_npdt_attack_pairs, 3 },
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

static constexpr sub_record_rule_t race_sub_rules[] = {
	{ "RADT", 140, element_wise_merge },
};

static constexpr sub_record_rule_t generic_sub_rules[] = {
	{ "AIDT", 12, element_wise_merge },
};

static constexpr record_behavior_t behavior_table[] = {
	{ "CELL", decode_mode_t::cell, copy_strategy_t::header_and_selected_group, nullptr, 0, &cell_wildcard, nullptr, 0, true },
	{ "LEVI", decode_mode_t::leveled, copy_strategy_t::whole_record, levi_sub_rules, 1, nullptr, nullptr, 0 },
	{ "LEVC", decode_mode_t::leveled, copy_strategy_t::whole_record, levi_sub_rules, 1, nullptr, nullptr, 0 },
	{ "FACT", decode_mode_t::faction, copy_strategy_t::whole_record, fact_sub_rules, 1, nullptr, nullptr, 0 },
	{ "CONT", decode_mode_t::container, copy_strategy_t::whole_record, cont_sub_rules, 1, nullptr, nullptr, 0 },
	{ "BSGN", decode_mode_t::container, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "RACE", decode_mode_t::container, copy_strategy_t::whole_record, race_sub_rules, 1, nullptr, nullptr, 0 },
	{ "NPC_", decode_mode_t::container, copy_strategy_t::whole_record, npc_sub_rules, 4, nullptr, nullptr, 0 },
	{ "CREA",
	  decode_mode_t::container,
	  copy_strategy_t::whole_record,
	  crea_sub_rules,
	  4,
	  nullptr,
	  crea_paired_rules,
	  1 },
	{ "WEAP", decode_mode_t::generic, copy_strategy_t::whole_record, weap_sub_rules, 1, nullptr, nullptr, 0 },
	{ "ARMO", decode_mode_t::armor, copy_strategy_t::whole_record, armo_sub_rules, 1, nullptr, nullptr, 0 },
	{ "CLOT", decode_mode_t::armor, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "DIAL", decode_mode_t::dial, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "INFO", decode_mode_t::info, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
};

static constexpr record_behavior_t generic_behavior = {
	"*", decode_mode_t::generic, copy_strategy_t::whole_record, generic_sub_rules, 1, nullptr, nullptr, 0
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
