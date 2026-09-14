#pragma once
#include <cstddef>
#include <set>
#include <string>
#include <vector>

enum class sub_rule_flag_t : unsigned
{
	none = 0,
	skip_non_existent = 1 << 1,
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

enum class merge_strategy_t
{
	generic,
	cell_refs,
	armor_parts,
	no_merge
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
	bool atomic_groups = false;
	merge_strategy_t merge_strategy = merge_strategy_t::generic;
	bool enam_effect_list = false;
	bool merge_excluded = false;
	const char * leveled_item_sub_type = nullptr;
	const char * const * keyed_list_sub_types = nullptr;
	size_t keyed_list_sub_type_count = 0;
};

enum class field_pair_role_t
{
	none,
	min_bound,
	max_bound
};

const record_behavior_t * find_record_behavior(const std::string & record_type);
const sub_record_rule_t * find_sub_record_rule(
    const record_behavior_t * behavior,
    const std::string & sub_type,
    size_t data_size);
field_pair_role_t find_field_pair_role(
    const std::string & record_type,
    const std::string & sub_type,
    size_t field_offset);

bool is_repeatable_sub_record(const std::string & record_type, const std::string & sub_type);

bool is_keyed_list_sub_type(const std::string & record_type, const std::string & sub_type);
merge_strategy_t merge_strategy_for(const std::string & record_type);
bool is_enam_effect_list(const std::string & record_type);
bool is_merge_excluded(const std::string & record_type);
decode_mode_t decode_mode_for(const std::string & record_type);
const char * leveled_item_sub_type_for(const std::string & record_type);
