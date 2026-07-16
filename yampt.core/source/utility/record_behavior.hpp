#pragma once
#include <cstddef>
#include <set>
#include <string>

enum class sub_rule_flag_t : unsigned
{
	none = 0,
	ignore_conflict = 1 << 0,
	skip_non_existent = 1 << 1,
	exclude_from_merge = 1 << 2,
	skip_if_size_differs = 1 << 3,
	element_wise_merge = 1 << 4,
};

inline constexpr sub_rule_flag_t operator|(sub_rule_flag_t left, sub_rule_flag_t right)
{
	return static_cast<sub_rule_flag_t>(static_cast<unsigned>(left) | static_cast<unsigned>(right));
}

inline constexpr bool has_flag(sub_rule_flag_t value, sub_rule_flag_t flag)
{
	return (static_cast<unsigned>(value) & static_cast<unsigned>(flag)) != 0;
}

struct sub_record_rule_t
{
	const char * sub_type;
	size_t expected_size;
	sub_rule_flag_t flags;
};

struct field_pair_rule_t
{
	size_t min_offset;
	size_t max_offset;
	size_t field_size;
};

struct paired_merge_rule_t
{
	const char * sub_type;
	size_t expected_size;
	const field_pair_rule_t * pairs;
	size_t pair_count;
};

enum class decode_mode_t
{
	generic,
	cell,
	leveled,
	faction,
	container,
	armor,
	info,
	dial
};

enum class copy_strategy_t
{
	whole_record,
	header_and_selected_group
};

struct record_behavior_t
{
	const char * record_type;
	decode_mode_t decode_mode;
	copy_strategy_t copy_strategy;
	const sub_record_rule_t * sub_rules;
	size_t sub_rule_count;
	const sub_record_rule_t * wildcard_rule;
	const paired_merge_rule_t * paired_rules;
	size_t paired_rule_count;
};

const record_behavior_t * find_record_behavior(const std::string & record_type);
const sub_record_rule_t * find_sub_record_rule(
    const record_behavior_t * behavior,
    const std::string & sub_type,
    size_t data_size);

struct sub_record_user_policy_t
{
	bool ignore_conflict = false;
	bool exclude_from_merge = false;
	bool skip_if_missing = false;
};

sub_record_user_policy_t find_user_policy(
    const std::string & record_type,
    const std::string & sub_type,
    const std::set<std::string> & ignore_conflict_subs,
    const std::set<std::string> & exclude_from_merge_subs,
    const std::set<std::string> & skip_if_missing_subs);
