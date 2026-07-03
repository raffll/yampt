#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct merge_input_t
{
	std::string rec_type;
	std::string record_id;
	std::vector<std::string> version_contents;
};

struct merge_result_t
{
	bool changed = false;
	std::string content;
};

struct sub_record_entry_t
{
	std::string type;
	std::string data;
};

using sub_record_sequence_t = std::vector<sub_record_entry_t>;
using leveled_list_input_t = merge_input_t;

struct frmr_group_t
{
	uint32_t frmr_index;
	sub_record_sequence_t sub_records;
};

struct cell_partition_t
{
	sub_record_sequence_t header;
	std::vector<frmr_group_t> groups;
};

using frmr_map_t = std::map<uint32_t, frmr_group_t>;

class sub_record_merge_t
{
public:
	static merge_result_t merge(const merge_input_t & input);

	static sub_record_sequence_t parse_sub_records(const std::string & content);
	static std::string serialize_sub_record(const sub_record_entry_t & entry);
	static std::string reconstruct_record(const std::string & winner_content, const sub_record_sequence_t & output);

	static size_t find_occurrence_index(const sub_record_sequence_t & sequence, size_t index);
	static int find_by_type_and_occurrence(const sub_record_sequence_t & sequence, const std::string & type, size_t occurrence);

	static void apply_intermediate(
		sub_record_sequence_t & output,
		const sub_record_sequence_t & first,
		const sub_record_sequence_t & intermediate,
		const sub_record_sequence_t & winner,
		const std::string & rec_type);

	static bool needs_element_wise(const std::string & rec_type, const std::string & sub_type, size_t data_size);
	static std::string merge_bytes_three_way(const char * first, const char * inter, const char * winner, size_t size);

	static std::vector<std::string> collect_enam_data(const sub_record_sequence_t & sequence);
	static std::string merge_enam_slots(
		const std::vector<std::string> & first_enams,
		const std::vector<std::string> & inter_enams,
		const std::vector<std::string> & winner_enams);
	static bool is_enam_record_type(const std::string & rec_type);
	static sub_record_sequence_t replace_enam_entries(
		const sub_record_sequence_t & output,
		const std::string & merged_enam_data);

	static cell_partition_t partition_cell(const std::string & content);
	static frmr_map_t build_frmr_map(const std::vector<frmr_group_t> & groups);
	static uint32_t read_frmr_index(const sub_record_entry_t & frmr_entry);

private:
	static merge_result_t merge_generic(const merge_input_t & input);
	static merge_result_t merge_cell_refs(const merge_input_t & input);

	static void apply_intermediate_to_group(
		sub_record_sequence_t & output,
		const sub_record_sequence_t & first,
		const sub_record_sequence_t & intermediate,
		const sub_record_sequence_t & winner);
	static sub_record_sequence_t merge_frmr_group(
		const sub_record_sequence_t & first_subs,
		const sub_record_sequence_t & inter_subs,
		const sub_record_sequence_t & winner_subs);
	static std::string reconstruct_cell(
		const std::string & winner_content,
		const sub_record_sequence_t & header,
		const std::vector<frmr_group_t> & groups);
	static void collect_intermediate_additions(
		std::vector<frmr_group_t> & merged_groups,
		const std::vector<std::string> & versions,
		const frmr_map_t & first_map,
		const frmr_map_t & winner_map);
	static void merge_winner_frmr_groups(
		std::vector<frmr_group_t> & merged_groups,
		const std::vector<std::string> & versions,
		const frmr_map_t & first_map,
		const frmr_map_t & winner_map);
};

class leveled_list_merge_t
{
public:
	static merge_result_t merge(const merge_input_t & input);
};
