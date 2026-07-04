#pragma once

#include <decoder/sub_record_iter.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct slot_result_t;

struct alignment_rule_t
{
	std::string anchor_type;
	size_t anchor_size = 0;
	std::vector<std::string> trailing_types;

	enum class key_from_t { anchor, next, offset };
	key_from_t key_source = key_from_t::anchor;
	std::string key_neighbor_type;
	size_t key_offset = 0;
	size_t key_length = 0;
};

struct sub_slot_t
{
	std::string type;
	int occurrence = 0;
};

class content_alignment_t
{
public:
	static void align(
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    size_t col_count,
	    const std::vector<alignment_rule_t> & rules,
	    std::vector<sub_slot_t> & unified_slots,
	    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices);

	static void build_from_slot_result(
	    const slot_result_t & slot_result,
	    size_t col_count,
	    std::vector<sub_slot_t> & unified_slots,
	    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices);

	static void build_occurrence_based(
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    size_t col_count,
	    std::vector<sub_slot_t> & unified_slots,
	    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices);

	static void build_occurrence_from_ranges(
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    size_t col_count,
	    const std::vector<size_t> & col_start,
	    const std::vector<size_t> & col_end,
	    std::vector<sub_slot_t> & unified_slots);

private:
	struct aligned_group_t
	{
		std::string content_key;
		size_t anchor_idx;
		std::vector<std::pair<std::string, size_t>> trailing;
	};

	static std::string extract_key(
	    const sub_record_view_t & anchor,
	    const std::vector<sub_record_view_t> & subs,
	    size_t anchor_pos,
	    const alignment_rule_t & rule);

	static void collect_non_excluded(
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    size_t col_count,
	    const std::vector<alignment_rule_t> & rules,
	    std::vector<sub_slot_t> & unified_slots,
	    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices);

	static void scan_groups(
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    size_t col_count,
	    const alignment_rule_t & rule,
	    std::vector<std::vector<aligned_group_t>> & col_groups,
	    std::vector<std::string> & all_keys,
	    int merge_column = -1);

	static void emit_key_slots(
	    const std::string & key,
	    const std::vector<std::vector<aligned_group_t>> & col_groups,
	    size_t col_count,
	    const alignment_rule_t & rule,
	    std::vector<sub_slot_t> & unified_slots,
	    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices,
	    int merge_column = -1);

	static std::string resolve_trailing_type(
	    const std::string & key,
	    size_t trailing_index,
	    const std::vector<std::vector<aligned_group_t>> & col_groups,
	    size_t col_count,
	    const alignment_rule_t & rule);

	static void fill_key_indices(
	    const std::string & key,
	    const std::vector<std::vector<aligned_group_t>> & col_groups,
	    size_t col_count,
	    const alignment_rule_t & rule,
	    size_t max_trailing,
	    const std::vector<std::string> & trailing_slot_types,
	    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices,
	    int merge_column = -1);

	static void fit_merge_column(
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    const std::vector<std::vector<aligned_group_t>> & col_groups,
	    const std::vector<std::string> & all_keys,
	    int merge_column,
	    const alignment_rule_t & rule,
	    const std::vector<sub_slot_t> & unified_slots,
	    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices);
};
