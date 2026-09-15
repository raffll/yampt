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
	skip_emit = 1 << 5,
	merge_boundary = 1 << 6,
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

enum class merge_case_t
{
	none,
	last_only,
	three_way
};

enum class key_source_t
{
	whole_content,
	content_slice,
	identity_prefix,
	paired_name,
	index_value
};

struct keyed_list_spec_t
{
	const char * sub_type;
	size_t key_offset;
	size_t key_length;
	size_t minimum_record_size;
	key_source_t key_source = key_source_t::content_slice;
	const char * const * member_sub_types = nullptr;
	size_t member_sub_type_count = 0;
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

enum class read_only_reason_t
{
	editable,
	landscape_data
};

struct record_behavior_t
{
	const char * record_type = "*";
	merge_case_t merge = merge_case_t::three_way;
	decode_mode_t decode_mode = decode_mode_t::generic;
	copy_strategy_t copy_strategy = copy_strategy_t::whole_record;
	const sub_record_rule_t * sub_rules = nullptr;
	size_t sub_rule_count = 0;
	const sub_record_rule_t * wildcard_rule = nullptr;
	const paired_merge_rule_t * paired_rules = nullptr;
	size_t paired_rule_count = 0;
	bool enam_effect_list = false;
	bool merge_excluded = false;
	const char * leveled_item_sub_type = nullptr;
	const char * leveled_level_sub_type = nullptr;
	const keyed_list_spec_t * keyed_list_specs = nullptr;
	size_t keyed_list_spec_count = 0;
	const char * const * armor_part_sub_types = nullptr;
	size_t armor_part_sub_type_count = 0;
	const char * record_id_sub_type = "NAME";
	read_only_reason_t read_only_reason = read_only_reason_t::editable;
	bool allows_copy = true;
	bool allows_lock = true;
	bool allows_exclude = true;
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

const keyed_list_spec_t * keyed_list_specs_for(const std::string & record_type, size_t & spec_count);
bool is_armor_part_sub_type(const std::string & record_type, const std::string & sub_type);
bool is_enam_effect_list(const std::string & record_type);
bool is_merge_excluded(const std::string & record_type);
merge_case_t merge_case_for(const std::string & record_type);
decode_mode_t decode_mode_for(const std::string & record_type);
const char * leveled_item_sub_type_for(const std::string & record_type);
const char * leveled_level_sub_type_for(const std::string & record_type);
const char * record_id_sub_type_for(const std::string & record_type);
read_only_reason_t read_only_reason_for(const std::string & record_type);
bool record_allows_copy(const std::string & record_type);
bool record_allows_lock(const std::string & record_type);
bool record_allows_exclude(const std::string & record_type);
