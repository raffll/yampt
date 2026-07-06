#include "record_behavior.hpp"
#include <cstring>

using enum sub_rule_flag_t;

static constexpr sub_record_rule_t cell_wildcard = { "*", 0, skip_non_existent };

static constexpr sub_record_rule_t cell_sub_rules[] = {
	{ "NAM0", 0, ignore_conflict | exclude_from_merge },
	{ "NAM9", 0, skip_non_existent | exclude_from_merge },
};

static constexpr field_pair_rule_t crea_npdt_attack_pairs[] = {
	{ 68, 70, 2 },
	{ 72, 74, 2 },
	{ 76, 78, 2 },
};

static constexpr paired_merge_rule_t crea_paired_rules[] = {
	{ "NPDT", 96, crea_npdt_attack_pairs, 3 },
};

static constexpr sub_record_rule_t npc_sub_rules[] = {
	{ "NPDT", 52, skip_if_size_differs | element_wise_merge },
	{ "NPDT", 12, skip_if_size_differs | element_wise_merge },
};

static constexpr sub_record_rule_t crea_sub_rules[] = {
	{ "NPDT", 96, element_wise_merge },
	{ "AI_W", 14, element_wise_merge },
};

static constexpr sub_record_rule_t generic_sub_rules[] = {
	{ "AIDT", 12, element_wise_merge },
};

static constexpr record_behavior_t behavior_table[] = {
	{ "CELL",
	  decode_mode_t::cell,
	  copy_strategy_t::header_and_selected_group,
	  cell_sub_rules,
	  2,
	  &cell_wildcard,
	  nullptr,
	  0 },
	{ "LEVI", decode_mode_t::leveled, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "LEVC", decode_mode_t::leveled, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "FACT", decode_mode_t::faction, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "CONT", decode_mode_t::container, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "BSGN", decode_mode_t::container, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "RACE", decode_mode_t::container, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "NPC_", decode_mode_t::container, copy_strategy_t::whole_record, npc_sub_rules, 2, nullptr, nullptr, 0 },
	{ "CREA",
	  decode_mode_t::container,
	  copy_strategy_t::whole_record,
	  crea_sub_rules,
	  2,
	  nullptr,
	  crea_paired_rules,
	  1 },
	{ "ARMO", decode_mode_t::armor, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
	{ "CLOT", decode_mode_t::armor, copy_strategy_t::whole_record, nullptr, 0, nullptr, nullptr, 0 },
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
