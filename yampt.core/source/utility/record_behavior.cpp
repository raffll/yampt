#include "record_behavior.hpp"
#include <cstring>
#include <set>

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
	{ "AIDT", 12, element_wise_merge },
};

static constexpr sub_record_rule_t crea_sub_rules[] = {
	{ "NPDT", 96, element_wise_merge },
	{ "AI_W", 14, element_wise_merge },
	{ "AIDT", 12, element_wise_merge },
};

static constexpr sub_record_rule_t weap_sub_rules[] = {
	{ "WPDT", 32, element_wise_merge },
};

static constexpr sub_record_rule_t armo_sub_rules[] = {
	{ "AODT", 24, element_wise_merge },
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
	{ "NPC_", decode_mode_t::container, copy_strategy_t::whole_record, npc_sub_rules, 3, nullptr, nullptr, 0 },
	{ "CREA",
	  decode_mode_t::container,
	  copy_strategy_t::whole_record,
	  crea_sub_rules,
	  3,
	  nullptr,
	  crea_paired_rules,
	  1 },
	{ "WEAP", decode_mode_t::generic, copy_strategy_t::whole_record, weap_sub_rules, 1, nullptr, nullptr, 0 },
	{ "ARMO", decode_mode_t::armor, copy_strategy_t::whole_record, armo_sub_rules, 1, nullptr, nullptr, 0 },
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

static bool matches_rule_set(
    const std::string & record_type,
    const std::string & sub_type,
    const std::set<std::string> & rules)
{
	const auto specific_key = record_type + ":" + sub_type;
	if (rules.count(specific_key))
		return true;

	const auto wildcard_key = record_type + ":*";
	if (rules.count(wildcard_key))
		return true;

	return false;
}

sub_record_user_policy_t find_user_policy(
    const std::string & record_type,
    const std::string & sub_type,
    const std::set<std::string> & ignore_conflict_subs,
    const std::set<std::string> & exclude_from_merge_subs,
    const std::set<std::string> & skip_if_missing_subs)
{
	sub_record_user_policy_t policy;
	policy.ignore_conflict = matches_rule_set(record_type, sub_type, ignore_conflict_subs);
	policy.exclude_from_merge = matches_rule_set(record_type, sub_type, exclude_from_merge_subs);
	policy.skip_if_missing = matches_rule_set(record_type, sub_type, skip_if_missing_subs);
	return policy;
}
