#include "sub_record_merge.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../utility/tools.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct sub_record_entry_t
{
	std::string type;
	std::string data;
};

using sub_record_sequence_t = std::vector<sub_record_entry_t>;

static sub_record_sequence_t parse_sub_records(const std::string & content)
{
	sub_record_sequence_t sequence;
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
		sequence.push_back({view.type, std::string(view.data, view.size)});

	return sequence;
}

static std::string serialize_sub_record(const sub_record_entry_t & entry)
{
	const auto size_bytes = tools_t::convert_uint_to_string_byte_array(entry.data.size());
	return entry.type + size_bytes + entry.data;
}

static std::string reconstruct_record(const std::string & winner_content, const sub_record_sequence_t & output)
{
	std::string body;
	for (const auto & entry : output)
		body += serialize_sub_record(entry);

	std::string result = winner_content.substr(0, 16);
	const auto body_size = tools_t::convert_uint_to_string_byte_array(body.size());
	result.replace(4, 4, body_size);
	result += body;
	return result;
}

static size_t find_occurrence_index(const sub_record_sequence_t & sequence, size_t index)
{
	size_t count = 0;
	const auto & target_type = sequence[index].type;

	for (size_t i = 0; i < index; ++i)
	{
		if (sequence[i].type == target_type)
			++count;
	}

	return count;
}

static int find_by_type_and_occurrence(const sub_record_sequence_t & sequence, const std::string & type, size_t occurrence)
{
	size_t count = 0;

	for (size_t i = 0; i < sequence.size(); ++i)
	{
		if (sequence[i].type != type)
			continue;

		if (count == occurrence)
			return static_cast<int>(i);

		++count;
	}

	return -1;
}

static bool needs_element_wise(const std::string & rec_type, const std::string & sub_type, size_t data_size)
{
	if (rec_type == "NPC_" && sub_type == "NPDT" && (data_size == 52 || data_size == 12))
		return true;

	if (rec_type == "CREA" && sub_type == "NPDT" && data_size == 96)
		return true;

	if (rec_type == "CREA" && sub_type == "AI_W" && data_size == 14)
		return true;

	return false;
}

static std::string merge_bytes_three_way(const char * first, const char * inter, const char * winner, size_t size)
{
	std::string result(winner, size);

	for (size_t offset = 0; offset < size; ++offset)
	{
		if (inter[offset] != first[offset] && winner[offset] == first[offset])
			result[offset] = inter[offset];
	}

	return result;
}

static constexpr size_t enam_slot_size = 24;

static std::vector<std::string> collect_enam_data(const sub_record_sequence_t & sequence)
{
	std::vector<std::string> slots;

	for (const auto & entry : sequence)
	{
		if (entry.type == "ENAM")
			slots.push_back(entry.data);
	}

	return slots;
}

static std::string merge_enam_slots(
	const std::vector<std::string> & first_enams,
	const std::vector<std::string> & inter_enams,
	const std::vector<std::string> & winner_enams)
{
	std::string result;

	for (size_t slot = 0; slot < winner_enams.size(); ++slot)
	{
		if (slot < first_enams.size() &&
			winner_enams[slot] == first_enams[slot] &&
			slot < inter_enams.size() &&
			inter_enams[slot] != first_enams[slot])
		{
			result += inter_enams[slot];
			continue;
		}

		result += winner_enams[slot];
	}

	for (size_t slot = first_enams.size(); slot < inter_enams.size(); ++slot)
	{
		if (slot >= winner_enams.size())
			result += inter_enams[slot];
	}

	return result;
}

static bool is_enam_record_type(const std::string & rec_type)
{
	return rec_type == "ENCH" || rec_type == "SPEL" || rec_type == "ALCH";
}

static void apply_intermediate(
	sub_record_sequence_t & output,
	const sub_record_sequence_t & first,
	const sub_record_sequence_t & intermediate,
	const sub_record_sequence_t & winner,
	const std::string & rec_type)
{
	for (size_t i = 0; i < intermediate.size(); ++i)
	{
		if (is_enam_record_type(rec_type) && intermediate[i].type == "ENAM")
			continue;

		const auto occurrence = find_occurrence_index(intermediate, i);
		const auto first_idx = find_by_type_and_occurrence(first, intermediate[i].type, occurrence);

		if (first_idx < 0)
			continue;

		if (intermediate[i].data == first[first_idx].data)
			continue;

		const auto winner_idx = find_by_type_and_occurrence(winner, intermediate[i].type, occurrence);

		if (winner_idx < 0)
			continue;

		const auto output_idx = find_by_type_and_occurrence(output, intermediate[i].type, occurrence);

		if (output_idx < 0)
			continue;

		if (needs_element_wise(rec_type, intermediate[i].type, first[first_idx].data.size()))
		{
			output[output_idx].data = merge_bytes_three_way(
				first[first_idx].data.data(),
				intermediate[i].data.data(),
				winner[winner_idx].data.data(),
				first[first_idx].data.size());
			continue;
		}

		if (winner[winner_idx].data != first[first_idx].data)
			continue;

		output[output_idx].data = intermediate[i].data;
	}
}

merge_result_t sub_record_merge_t::merge(const merge_input_t & input)
{
	if (input.rec_type == "CELL")
		return merge_cell_refs(input);

	return merge_generic(input);
}

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

static uint32_t read_frmr_index(const sub_record_entry_t & frmr_entry)
{
	return static_cast<uint32_t>(
		tools_t::convert_string_byte_array_to_uint(frmr_entry.data.substr(0, 4)));
}

static cell_partition_t partition_cell(const std::string & content)
{
	cell_partition_t result;
	sub_record_iter_t iter(content);
	sub_record_view_t view;
	bool in_frmr = false;

	while (iter.next(view))
	{
		sub_record_entry_t entry{view.type, std::string(view.data, view.size)};

		if (view.type == "FRMR")
		{
			in_frmr = true;
			frmr_group_t group;
			group.frmr_index = read_frmr_index(entry);
			group.sub_records.push_back(std::move(entry));
			result.groups.push_back(std::move(group));
			continue;
		}

		if (!in_frmr)
			result.header.push_back(std::move(entry));
		else
			result.groups.back().sub_records.push_back(std::move(entry));
	}

	return result;
}

using frmr_map_t = std::map<uint32_t, frmr_group_t>;

static frmr_map_t build_frmr_map(const std::vector<frmr_group_t> & groups)
{
	frmr_map_t result;

	for (const auto & group : groups)
		result.emplace(group.frmr_index, group);

	return result;
}

static void apply_intermediate_to_group(
	sub_record_sequence_t & output,
	const sub_record_sequence_t & first,
	const sub_record_sequence_t & intermediate,
	const sub_record_sequence_t & winner)
{
	for (size_t i = 0; i < intermediate.size(); ++i)
	{
		const auto occurrence = find_occurrence_index(intermediate, i);
		const auto first_idx = find_by_type_and_occurrence(first, intermediate[i].type, occurrence);

		if (first_idx < 0)
			continue;

		if (intermediate[i].data == first[first_idx].data)
			continue;

		const auto winner_idx = find_by_type_and_occurrence(winner, intermediate[i].type, occurrence);

		if (winner_idx < 0)
			continue;

		const auto output_idx = find_by_type_and_occurrence(output, intermediate[i].type, occurrence);

		if (output_idx < 0)
			continue;

		if (winner[winner_idx].data != first[first_idx].data)
			continue;

		output[output_idx].data = intermediate[i].data;
	}
}

static sub_record_sequence_t merge_frmr_group(
	const sub_record_sequence_t & first_subs,
	const sub_record_sequence_t & inter_subs,
	const sub_record_sequence_t & winner_subs)
{
	auto output = winner_subs;
	apply_intermediate_to_group(output, first_subs, inter_subs, winner_subs);
	return output;
}

static std::string reconstruct_cell(
	const std::string & winner_content,
	const sub_record_sequence_t & header,
	const std::vector<frmr_group_t> & groups)
{
	std::string body;

	for (const auto & entry : header)
		body += serialize_sub_record(entry);

	for (const auto & group : groups)
	{
		for (const auto & entry : group.sub_records)
			body += serialize_sub_record(entry);
	}

	std::string result = winner_content.substr(0, 16);
	const auto body_size = tools_t::convert_uint_to_string_byte_array(body.size());
	result.replace(4, 4, body_size);
	result += body;
	return result;
}

static void collect_intermediate_additions(
	std::vector<frmr_group_t> & merged_groups,
	const std::vector<std::string> & versions,
	const frmr_map_t & first_map,
	const frmr_map_t & winner_map)
{
	for (size_t v = versions.size() - 2; v >= 1; --v)
	{
		const auto inter_part = partition_cell(versions[v]);

		for (const auto & group : inter_part.groups)
		{
			if (first_map.count(group.frmr_index) > 0)
				continue;

			if (winner_map.count(group.frmr_index) > 0)
				continue;

			bool already_added = false;

			for (const auto & existing : merged_groups)
			{
				if (existing.frmr_index == group.frmr_index)
				{
					already_added = true;
					break;
				}
			}

			if (!already_added)
				merged_groups.push_back(group);
		}
	}
}

static void merge_winner_frmr_groups(
	std::vector<frmr_group_t> & merged_groups,
	const std::vector<std::string> & versions,
	const frmr_map_t & first_map,
	const frmr_map_t & winner_map)
{
	for (const auto & [index, winner_group] : winner_map)
	{
		auto it_first = first_map.find(index);

		if (it_first == first_map.end())
		{
			merged_groups.push_back(winner_group);
			continue;
		}

		auto merged_subs = winner_group.sub_records;

		for (size_t v = versions.size() - 2; v >= 1; --v)
		{
			const auto inter_part = partition_cell(versions[v]);
			const auto inter_map = build_frmr_map(inter_part.groups);
			auto it_inter = inter_map.find(index);

			if (it_inter == inter_map.end())
				continue;

			merged_subs = merge_frmr_group(
				it_first->second.sub_records, it_inter->second.sub_records, merged_subs);
		}

		merged_groups.push_back({index, std::move(merged_subs)});
	}
}

merge_result_t sub_record_merge_t::merge_cell_refs(const merge_input_t & input)
{
	const auto & versions = input.version_contents;

	if (versions.size() < 3)
		return {false, versions.back()};

	const auto & first_content = versions.front();
	const auto & winner_content = versions.back();

	const auto first_part = partition_cell(first_content);
	const auto winner_part = partition_cell(winner_content);

	auto merged_header = winner_part.header;

	for (size_t v = versions.size() - 2; v >= 1; --v)
	{
		const auto inter_part = partition_cell(versions[v]);
		apply_intermediate(
			merged_header, first_part.header, inter_part.header, winner_part.header, "CELL");
	}

	const auto first_map = build_frmr_map(first_part.groups);
	const auto winner_map = build_frmr_map(winner_part.groups);

	std::vector<frmr_group_t> merged_groups;
	merge_winner_frmr_groups(merged_groups, versions, first_map, winner_map);
	collect_intermediate_additions(merged_groups, versions, first_map, winner_map);

	std::sort(merged_groups.begin(), merged_groups.end(),
		[](const frmr_group_t & lhs, const frmr_group_t & rhs)
		{
			return lhs.frmr_index < rhs.frmr_index;
		});

	const auto result = reconstruct_cell(winner_content, merged_header, merged_groups);

	if (result == winner_content)
		return {false, winner_content};

	return {true, result};
}

static sub_record_sequence_t replace_enam_entries(
	const sub_record_sequence_t & output,
	const std::string & merged_enam_data)
{
	sub_record_sequence_t result;

	for (const auto & entry : output)
	{
		if (entry.type != "ENAM")
			result.push_back(entry);
	}

	for (size_t offset = 0; offset + enam_slot_size <= merged_enam_data.size(); offset += enam_slot_size)
		result.push_back({"ENAM", merged_enam_data.substr(offset, enam_slot_size)});

	return result;
}

merge_result_t sub_record_merge_t::merge_generic(const merge_input_t & input)
{
	const auto & versions = input.version_contents;

	if (versions.size() < 3)
		return {false, versions.back()};

	const auto & first_content = versions.front();
	const auto & winner_content = versions.back();

	const auto first_subs = parse_sub_records(first_content);
	const auto winner_subs = parse_sub_records(winner_content);
	auto output = winner_subs;

	for (size_t v = versions.size() - 2; v >= 1; --v)
	{
		const auto inter_subs = parse_sub_records(versions[v]);
		apply_intermediate(output, first_subs, inter_subs, winner_subs, input.rec_type);
	}

	if (is_enam_record_type(input.rec_type))
	{
		const auto first_enams = collect_enam_data(first_subs);
		auto merged_enams = collect_enam_data(winner_subs);
		std::string merged_enam_data;

		for (const auto & slot : merged_enams)
			merged_enam_data += slot;

		for (size_t v = versions.size() - 2; v >= 1; --v)
		{
			const auto inter_subs = parse_sub_records(versions[v]);
			const auto inter_enams = collect_enam_data(inter_subs);

			std::vector<std::string> current_slots;
			for (size_t offset = 0; offset + enam_slot_size <= merged_enam_data.size(); offset += enam_slot_size)
				current_slots.push_back(merged_enam_data.substr(offset, enam_slot_size));

			merged_enam_data = merge_enam_slots(first_enams, inter_enams, current_slots);
		}

		output = replace_enam_entries(output, merged_enam_data);
	}

	const auto result = reconstruct_record(winner_content, output);

	if (result == winner_content)
		return {false, winner_content};

	return {true, result};
}
