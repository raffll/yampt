#include "sub_record_merge.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../utility/app_logger.hpp"
#include "../utility/string_utils.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <set>

static constexpr size_t enam_slot_size = 24;

sub_record_sequence_t sub_record_merge_t::parse_sub_records(const std::string & content)
{
	sub_record_sequence_t sequence;
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
		sequence.push_back({ view.type, std::string(view.data, view.size) });

	return sequence;
}

std::string sub_record_merge_t::serialize_sub_record(const sub_record_entry_t & entry)
{
	const auto size_bytes = domain_types::convert_uint_to_string_byte_array(entry.data.size());
	return entry.type + size_bytes + entry.data;
}

std::string sub_record_merge_t::reconstruct_record(
    const std::string & winner_content,
    const sub_record_sequence_t & output)
{
	std::string body;
	for (const auto & entry : output)
		body += serialize_sub_record(entry);

	std::string result = winner_content.substr(0, 16);
	const auto body_size = domain_types::convert_uint_to_string_byte_array(body.size());
	result.replace(4, 4, body_size);
	result += body;
	return result;
}

size_t sub_record_merge_t::find_occurrence_index(const sub_record_sequence_t & sequence, size_t index)
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

int sub_record_merge_t::find_by_type_and_occurrence(
    const sub_record_sequence_t & sequence,
    const std::string & type,
    size_t occurrence)
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

bool sub_record_merge_t::needs_element_wise(
    const std::string & rec_type,
    const std::string & sub_type,
    size_t data_size)
{
	if (rec_type == "NPC_" && sub_type == "NPDT" && (data_size == 52 || data_size == 12))
		return true;

	if (rec_type == "CREA" && sub_type == "NPDT" && data_size == 96)
		return true;

	if (rec_type == "CREA" && sub_type == "AI_W" && data_size == 14)
		return true;

	if (sub_type == "AIDT" && data_size == 12)
		return true;

	return false;
}

std::string sub_record_merge_t::merge_bytes_three_way(
    const char * first,
    const char * inter,
    const char * winner,
    size_t size)
{
	std::string result(winner, size);

	for (size_t offset = 0; offset < size; ++offset)
	{
		if (inter[offset] != first[offset] && winner[offset] == first[offset])
			result[offset] = inter[offset];
	}

	return result;
}

std::vector<std::string> sub_record_merge_t::collect_enam_data(const sub_record_sequence_t & sequence)
{
	std::vector<std::string> slots;

	for (const auto & entry : sequence)
	{
		if (entry.type == "ENAM")
			slots.push_back(entry.data);
	}

	return slots;
}

static constexpr size_t enam_mag_min_offset = 12;
static constexpr size_t enam_mag_max_offset = 16;
static constexpr size_t enam_field_size = 4;

struct field_pair_t
{
	size_t min_offset;
	size_t max_offset;
	size_t length;
};

static constexpr field_pair_t crea_npdt_attack_pairs[] = {
	{ 68, 70, 2 },
	{ 72, 74, 2 },
	{ 76, 78, 2 },
};

static bool field_changed(const std::string & source, const std::string & base, size_t offset, size_t length)
{
	return source.compare(offset, length, base, offset, length) != 0;
}

static void fix_paired_fields(
    std::string & merged,
    const std::string & first,
    const std::string & inter,
    const std::string & winner,
    const field_pair_t * pairs,
    size_t pair_count)
{
	for (size_t p = 0; p < pair_count; ++p)
	{
		const auto & pair = pairs[p];
		const bool inter_changed_min = field_changed(inter, first, pair.min_offset, pair.length);
		const bool inter_changed_max = field_changed(inter, first, pair.max_offset, pair.length);
		const bool winner_changed_min = field_changed(winner, first, pair.min_offset, pair.length);
		const bool winner_changed_max = field_changed(winner, first, pair.max_offset, pair.length);

		if (winner_changed_min || winner_changed_max)
		{
			merged.replace(pair.min_offset, pair.length, winner, pair.min_offset, pair.length);
			merged.replace(pair.max_offset, pair.length, winner, pair.max_offset, pair.length);
			continue;
		}

		if (inter_changed_min || inter_changed_max)
		{
			merged.replace(pair.min_offset, pair.length, inter, pair.min_offset, pair.length);
			merged.replace(pair.max_offset, pair.length, inter, pair.max_offset, pair.length);
		}
	}
}

static constexpr field_pair_t enam_magnitude_pair = { 12, 16, 4 };

static void fix_magnitude_pair(
    std::string & result,
    size_t slot_offset,
    const std::string & first,
    const std::string & inter,
    const std::string & winner)
{
	std::string merged_slot = result.substr(slot_offset, enam_slot_size);
	fix_paired_fields(merged_slot, first, inter, winner, &enam_magnitude_pair, 1);
	result.replace(slot_offset, enam_slot_size, merged_slot);
}

std::string sub_record_merge_t::merge_enam_slots(
    const std::vector<std::string> & first_enams,
    const std::vector<std::string> & inter_enams,
    const std::vector<std::string> & winner_enams)
{
	std::string result;

	for (size_t slot = 0; slot < winner_enams.size(); ++slot)
	{
		if (slot >= first_enams.size() || slot >= inter_enams.size())
		{
			result += winner_enams[slot];
			continue;
		}

		if (inter_enams[slot] == first_enams[slot])
		{
			result += winner_enams[slot];
			continue;
		}

		result += merge_bytes_three_way(
		    first_enams[slot].data(), inter_enams[slot].data(), winner_enams[slot].data(), enam_slot_size);

		fix_magnitude_pair(
		    result, result.size() - enam_slot_size, first_enams[slot], inter_enams[slot], winner_enams[slot]);
	}

	for (size_t slot = first_enams.size(); slot < inter_enams.size(); ++slot)
	{
		if (slot >= winner_enams.size())
			result += inter_enams[slot];
	}

	return result;
}

bool sub_record_merge_t::is_enam_record_type(const std::string & rec_type)
{
	return rec_type == "ENCH" || rec_type == "SPEL" || rec_type == "ALCH";
}

sub_record_sequence_t sub_record_merge_t::replace_enam_entries(
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
		result.push_back({ "ENAM", merged_enam_data.substr(offset, enam_slot_size) });

	return result;
}

void sub_record_merge_t::apply_intermediate(
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

		if (intermediate[i].type == "NPCO")
			continue;

		const auto occurrence = find_occurrence_index(intermediate, i);
		const auto first_idx = find_by_type_and_occurrence(first, intermediate[i].type, occurrence);

		if (first_idx < 0)
		{
			const auto winner_idx = find_by_type_and_occurrence(winner, intermediate[i].type, occurrence);
			if (winner_idx < 0)
			{
				const auto output_idx = find_by_type_and_occurrence(output, intermediate[i].type, occurrence);
				if (output_idx < 0)
					output.push_back(intermediate[i]);
			}

			continue;
		}

		if (intermediate[i].data == first[first_idx].data)
			continue;

		const auto winner_idx = find_by_type_and_occurrence(winner, intermediate[i].type, occurrence);

		if (winner_idx < 0)
			continue;

		const auto output_idx = find_by_type_and_occurrence(output, intermediate[i].type, occurrence);

		if (output_idx < 0)
			continue;

		if (needs_element_wise(rec_type, intermediate[i].type, first[first_idx].data.size()) &&
		    intermediate[i].data.size() == first[first_idx].data.size() &&
		    winner[winner_idx].data.size() == first[first_idx].data.size())
		{
			output[output_idx].data = merge_bytes_three_way(
			    first[first_idx].data.data(),
			    intermediate[i].data.data(),
			    winner[winner_idx].data.data(),
			    first[first_idx].data.size());

			if (rec_type == "CREA" && intermediate[i].type == "NPDT" && first[first_idx].data.size() == 96)
			{
				fix_paired_fields(
				    output[output_idx].data,
				    first[first_idx].data,
				    intermediate[i].data,
				    winner[winner_idx].data,
				    crea_npdt_attack_pairs,
				    3);
			}

			continue;
		}

		if (winner[winner_idx].data != first[first_idx].data)
			continue;

		if (output[output_idx].data != first[first_idx].data)
			continue;

		output[output_idx].data = intermediate[i].data;
	}
}

merge_result_t sub_record_merge_t::merge(const merge_input_t & input)
{
	if (input.rec_type == "CELL")
		return { false, input.version_contents.back() };

	if (input.rec_type == "SCPT")
		return { false, input.version_contents.back() };

	return merge_generic(input);
}

// NOTE: merge_cell_refs is intentionally kept as dead code.
// The engine handles FRMR merging natively at runtime.
// This code may be needed in the future for non-engine use cases.

uint32_t sub_record_merge_t::read_frmr_index(const sub_record_entry_t & frmr_entry)
{
	return static_cast<uint32_t>(domain_types::convert_string_byte_array_to_uint(frmr_entry.data.substr(0, 4)));
}

cell_partition_t sub_record_merge_t::partition_cell(const std::string & content)
{
	cell_partition_t result;
	sub_record_iter_t iter(content);
	sub_record_view_t view;
	bool in_frmr = false;

	while (iter.next(view))
	{
		sub_record_entry_t entry { view.type, std::string(view.data, view.size) };

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

frmr_map_t sub_record_merge_t::build_frmr_map(const std::vector<frmr_group_t> & groups)
{
	frmr_map_t result;

	for (const auto & group : groups)
		result.emplace(group.frmr_index, group);

	return result;
}

void sub_record_merge_t::apply_intermediate_to_group(
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

sub_record_sequence_t sub_record_merge_t::merge_frmr_group(
    const sub_record_sequence_t & first_subs,
    const sub_record_sequence_t & inter_subs,
    const sub_record_sequence_t & winner_subs)
{
	auto output = winner_subs;
	apply_intermediate_to_group(output, first_subs, inter_subs, winner_subs);
	return output;
}

std::string sub_record_merge_t::reconstruct_cell(
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
	const auto body_size = domain_types::convert_uint_to_string_byte_array(body.size());
	result.replace(4, 4, body_size);
	result += body;
	return result;
}

void sub_record_merge_t::collect_intermediate_additions(
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

void sub_record_merge_t::merge_winner_frmr_groups(
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

			merged_subs = merge_frmr_group(it_first->second.sub_records, it_inter->second.sub_records, merged_subs);
		}

		merged_groups.push_back({ index, std::move(merged_subs) });
	}
}

merge_result_t sub_record_merge_t::merge_cell_refs(const merge_input_t & input)
{
	const auto & versions = input.version_contents;

	if (versions.size() < 3)
		return { false, versions.back() };

	const auto & first_content = versions.front();
	const auto & winner_content = versions.back();

	const auto first_part = partition_cell(first_content);
	const auto winner_part = partition_cell(winner_content);

	auto merged_header = winner_part.header;

	for (size_t v = versions.size() - 2; v >= 1; --v)
	{
		const auto inter_part = partition_cell(versions[v]);
		apply_intermediate(merged_header, first_part.header, inter_part.header, winner_part.header, "CELL");
	}

	const auto first_map = build_frmr_map(first_part.groups);
	const auto winner_map = build_frmr_map(winner_part.groups);

	std::vector<frmr_group_t> merged_groups;
	merge_winner_frmr_groups(merged_groups, versions, first_map, winner_map);
	collect_intermediate_additions(merged_groups, versions, first_map, winner_map);

	std::sort(
	    merged_groups.begin(),
	    merged_groups.end(),
	    [](const frmr_group_t & lhs, const frmr_group_t & rhs) { return lhs.frmr_index < rhs.frmr_index; });

	const auto result = reconstruct_cell(winner_content, merged_header, merged_groups);

	if (result == winner_content)
		return { false, winner_content };

	return { true, result };
}

static constexpr size_t npco_item_id_offset = 4;
static constexpr size_t npco_item_id_length = 32;
static constexpr size_t npco_sub_record_size = 36;

static std::string extract_npco_item_id(const sub_record_entry_t & entry)
{
	if (entry.data.size() < npco_sub_record_size)
		return {};

	auto item_id = entry.data.substr(npco_item_id_offset, npco_item_id_length);
	auto null_pos = item_id.find('\0');
	if (null_pos != std::string::npos)
		item_id.resize(null_pos);

	return item_id;
}

static std::vector<sub_record_entry_t> collect_npco_entries(const sub_record_sequence_t & sequence)
{
	std::vector<sub_record_entry_t> result;

	for (const auto & entry : sequence)
	{
		if (entry.type == "NPCO")
			result.push_back(entry);
	}

	return result;
}

static bool has_npco_entries(const sub_record_sequence_t & sequence)
{
	for (const auto & entry : sequence)
	{
		if (entry.type == "NPCO")
			return true;
	}

	return false;
}

static std::vector<sub_record_entry_t> merge_npco_items(
    const std::vector<sub_record_entry_t> & first_items,
    const std::vector<sub_record_entry_t> & inter_items,
    const std::vector<sub_record_entry_t> & winner_items,
    bool is_patch_intermediate)
{
	std::set<std::string> winner_ids;
	for (const auto & item : winner_items)
		winner_ids.insert(extract_npco_item_id(item));

	std::set<std::string> first_ids;
	for (const auto & item : first_items)
		first_ids.insert(extract_npco_item_id(item));

	auto result = winner_items;

	for (const auto & item : inter_items)
	{
		const auto item_id = extract_npco_item_id(item);
		if (item_id.empty())
			continue;

		if (winner_ids.count(item_id))
			continue;

		if (!first_ids.count(item_id))
		{
			result.push_back(item);
			winner_ids.insert(item_id);
		}
	}

	return result;
}

static sub_record_sequence_t replace_npco_entries(
    const sub_record_sequence_t & output,
    const std::vector<sub_record_entry_t> & merged_items)
{
	sub_record_sequence_t result;

	for (const auto & entry : output)
	{
		if (entry.type != "NPCO")
			result.push_back(entry);
	}

	for (const auto & item : merged_items)
		result.push_back(item);

	return result;
}

static size_t find_npdt_size(const sub_record_sequence_t & subs)
{
	for (const auto & entry : subs)
	{
		if (entry.type == "NPDT")
			return entry.data.size();
	}

	return 0;
}

static bool has_mismatched_npdt(const sub_record_sequence_t & first, const sub_record_sequence_t & inter)
{
	const auto first_size = find_npdt_size(first);
	const auto inter_size = find_npdt_size(inter);

	if (first_size == 0 || inter_size == 0)
		return false;

	return first_size != inter_size;
}

merge_result_t sub_record_merge_t::merge_generic(const merge_input_t & input)
{
	const auto & versions = input.version_contents;

	if (versions.size() < 3)
		return { false, versions.back() };

	const auto & first_content = versions.front();
	const auto & winner_content = versions.back();

	const auto first_subs = parse_sub_records(first_content);
	const auto winner_subs = parse_sub_records(winner_content);
	auto output = winner_subs;

	for (size_t v = versions.size() - 2; v >= 1; --v)
	{
		const auto inter_subs = parse_sub_records(versions[v]);

		if (input.rec_type == "NPC_" && has_mismatched_npdt(first_subs, inter_subs))
			continue;

		apply_intermediate(output, first_subs, inter_subs, winner_subs, input.rec_type);
	}

	if (is_enam_record_type(input.rec_type))
	{
		const auto first_enams = collect_enam_data(first_subs);
		std::string merged_enam_data;

		for (const auto & slot : collect_enam_data(winner_subs))
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

	if (has_npco_entries(first_subs))
	{
		const auto first_items = collect_npco_entries(first_subs);
		const auto winner_items = collect_npco_entries(winner_subs);

		int patch_version_idx = -1;
		for (size_t v = versions.size() - 2; v >= 1; --v)
		{
			if (input.patch_version_indices.count(v))
			{
				patch_version_idx = static_cast<int>(v);
				break;
			}
		}

		std::vector<sub_record_entry_t> base_items;
		if (patch_version_idx >= 0)
		{
			const auto patch_subs = parse_sub_records(versions[patch_version_idx]);
			base_items = collect_npco_entries(patch_subs);
		}
		else
		{
			base_items = winner_items;
		}

		auto merged_items = base_items;

		for (size_t v = versions.size() - 2; v >= 1; --v)
		{
			if (static_cast<int>(v) == patch_version_idx)
				continue;

			const auto inter_subs = parse_sub_records(versions[v]);
			const auto inter_items = collect_npco_entries(inter_subs);
			merged_items = merge_npco_items(first_items, inter_items, merged_items, false);
		}

		if (patch_version_idx < 0)
		{
			merged_items = merge_npco_items(first_items, winner_items, merged_items, false);
		}
		else
		{
			const auto winner_additions = collect_npco_entries(winner_subs);
			merged_items = merge_npco_items(first_items, winner_additions, merged_items, false);
		}

		if (merged_items != winner_items)
			output = replace_npco_entries(output, merged_items);
	}

	const auto result = reconstruct_record(winner_content, output);

	if (result == winner_content)
		return { false, winner_content };

	return { true, result };
}

// leveled_list_merge_t implementation

struct list_item_t
{
	std::string ident;
	uint16_t level;
};

static std::vector<list_item_t> extract_list_items(const std::string & content)
{
	std::vector<list_item_t> items;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	std::string current_id;

	while (iter.next(sub))
	{
		if (sub.type == "INAM" || sub.type == "CNAM")
		{
			current_id = std::string(sub.data, sub.size);
			current_id = string_utils::erase_null_chars(current_id);
			continue;
		}

		if (sub.type == "INTV" && !current_id.empty())
		{
			uint16_t level = 0;
			if (sub.size >= 2)
				std::memcpy(&level, sub.data, 2);

			items.push_back({ current_id, level });
			current_id.clear();
		}
	}

	return items;
}

static std::string extract_list_header(const std::string & content)
{
	std::string header_part;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;

	while (iter.next(sub))
	{
		if (sub.type == "INAM" || sub.type == "CNAM" || sub.type == "INTV")
			break;

		if (sub.type == "INDX")
			continue;

		header_part += sub.type;
		header_part += domain_types::convert_uint_to_string_byte_array(sub.size);
		header_part += std::string(sub.data, sub.size);
	}

	return header_part;
}

static std::string build_merged_list_record(
    const std::string & rec_type,
    const std::string & header_part,
    const std::vector<list_item_t> & merged_items)
{
	std::string indx_sub = "INDX";
	uint32_t item_count = static_cast<uint32_t>(merged_items.size());
	indx_sub += domain_types::convert_uint_to_string_byte_array(4);
	indx_sub += std::string(reinterpret_cast<const char *>(&item_count), 4);

	const std::string & item_sub_type = (rec_type == "LEVI") ? "INAM" : "CNAM";
	std::string items_part;
	for (const auto & item : merged_items)
	{
		std::string id_data = item.ident;
		id_data.push_back('\0');

		items_part += item_sub_type;
		items_part += domain_types::convert_uint_to_string_byte_array(id_data.size());
		items_part += id_data;

		items_part += "INTV";
		items_part += domain_types::convert_uint_to_string_byte_array(2);
		items_part += std::string(reinterpret_cast<const char *>(&item.level), 2);
	}

	std::string body = header_part + indx_sub + items_part;

	std::string record;
	record += rec_type;
	record += domain_types::convert_uint_to_string_byte_array(body.size());
	record += std::string(8, '\0');
	record += body;

	return record;
}

using item_count_map_t = std::map<std::pair<std::string, uint16_t>, size_t>;

static item_count_map_t build_item_count_map(const std::vector<list_item_t> & items)
{
	item_count_map_t counts;
	for (const auto & item : items)
		++counts[{ item.ident, item.level }];

	return counts;
}

static bool is_item_deleted(
    const std::pair<std::string, uint16_t> & item_key,
    const item_count_map_t & first_map,
    const std::vector<item_count_map_t> & non_first_maps)
{
	const auto it_first = first_map.find(item_key);
	if (it_first == first_map.end() || it_first->second == 0)
		return false;

	for (const auto & version_map : non_first_maps)
	{
		const auto it_ver = version_map.find(item_key);
		if (it_ver == version_map.end())
			return true;
	}

	return false;
}

static size_t compute_merged_count(
    const std::pair<std::string, uint16_t> & item_key,
    const std::vector<item_count_map_t> & non_first_maps)
{
	size_t max_count = 0;
	for (const auto & version_map : non_first_maps)
	{
		const auto it_ver = version_map.find(item_key);
		if (it_ver != version_map.end() && it_ver->second > max_count)
			max_count = it_ver->second;
	}

	return max_count;
}

static std::vector<list_item_t> build_merged_items(
    const item_count_map_t & first_map,
    const std::vector<item_count_map_t> & non_first_maps)
{
	std::set<std::pair<std::string, uint16_t>> all_keys;
	for (const auto & [key, count] : first_map)
		all_keys.insert(key);

	for (const auto & version_map : non_first_maps)
	{
		for (const auto & [key, count] : version_map)
			all_keys.insert(key);
	}

	std::vector<list_item_t> merged;
	for (const auto & item_key : all_keys)
	{
		if (is_item_deleted(item_key, first_map, non_first_maps))
			continue;

		const auto merged_count = compute_merged_count(item_key, non_first_maps);
		for (size_t i = 0; i < merged_count; ++i)
			merged.push_back({ item_key.first, item_key.second });
	}

	return merged;
}

static void sort_merged_items(std::vector<list_item_t> & items)
{
	std::sort(
	    items.begin(),
	    items.end(),
	    [](const list_item_t & left, const list_item_t & right)
	{
		if (left.level != right.level)
			return left.level < right.level;

		return left.ident < right.ident;
	});
}

merge_result_t leveled_list_merge_t::merge(const merge_input_t & input)
{
	const auto & versions = input.version_contents;
	if (versions.size() < 2)
		return { false, {} };

	const auto & first_content = versions.front();
	const auto first_map = build_item_count_map(extract_list_items(first_content));

	std::vector<item_count_map_t> non_first_maps;
	std::string winning_content;

	for (size_t vi = 1; vi < versions.size(); ++vi)
	{
		non_first_maps.push_back(build_item_count_map(extract_list_items(versions[vi])));
		winning_content = versions[vi];
	}

	if (non_first_maps.empty())
		return { false, {} };

	auto merged = build_merged_items(first_map, non_first_maps);
	sort_merged_items(merged);

	const auto header_part = extract_list_header(winning_content);
	const auto record = build_merged_list_record(input.rec_type, header_part, merged);

	if (record == winning_content)
		return { false, winning_content };

	return { true, record };
}
