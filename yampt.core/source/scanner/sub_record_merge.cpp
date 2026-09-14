#include "sub_record_merge.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../decoder/sub_record_schema.hpp"
#include "../utility/app_logger.hpp"
#include "../utility/record_behavior.hpp"
#include "../utility/string_utils.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <optional>
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

std::string sub_record_merge_t::filter_sub_records_by_rules(
    const std::string & rec_type,
    const std::string & content,
    const std::set<std::string> & ignored_sub_records)
{
	if (ignored_sub_records.empty())
		return content;

	const auto subs = parse_sub_records(content);
	sub_record_sequence_t filtered;
	bool in_reference_group = false;

	const auto specific_key = rec_type + ":";
	const auto wildcard_key = rec_type + ":*";

	for (const auto & entry : subs)
	{
		if (entry.type == "FRMR")
			in_reference_group = true;

		if (in_reference_group)
		{
			filtered.push_back(entry);
			continue;
		}

		if (ignored_sub_records.count(specific_key + entry.type) > 0)
			continue;

		if (ignored_sub_records.count(wildcard_key) > 0)
			continue;

		filtered.push_back(entry);
	}

	if (filtered.size() == subs.size())
		return content;

	return reconstruct_record(content, filtered);
}

std::vector<std::pair<std::string, int>> sub_record_merge_t::group_members_in_range(
    const std::string & content,
    int group_start,
    int group_end)
{
	std::vector<std::pair<std::string, int>> members;
	const auto subs = parse_sub_records(content);

	if (group_start < 0 || group_end > static_cast<int>(subs.size()))
		return members;

	for (int index = group_start; index < group_end; ++index)
		members.emplace_back(subs[static_cast<size_t>(index)].type, index);

	return members;
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

static bool is_flags_field(const field_def_t & field)
{
	return field.type == field_type_t::flags_u8 || field.type == field_type_t::flags_u16 ||
	       field.type == field_type_t::flags_u32;
}

static size_t flag_byte_width(const field_def_t & field)
{
	if (field.type == field_type_t::flags_u8)
		return 1;

	if (field.type == field_type_t::flags_u16)
		return 2;

	return 4;
}

static void merge_field_bits(std::string & result, const sub_record_merge_t::field_merge_input_t & input, const field_def_t & field)
{
	const size_t width = flag_byte_width(field);

	for (size_t byte_index = 0; byte_index < width; ++byte_index)
	{
		const size_t offset = field.offset + byte_index;
		if (offset >= input.size)
			return;

		const unsigned char first_byte = static_cast<unsigned char>(input.first[offset]);
		const unsigned char inter_byte = static_cast<unsigned char>(input.inter[offset]);
		const unsigned char winner_byte = static_cast<unsigned char>(input.winner[offset]);
		const unsigned char current_byte = static_cast<unsigned char>(input.current[offset]);

		unsigned char merged = current_byte;
		for (int bit = 0; bit < 8; ++bit)
		{
			const unsigned char mask = static_cast<unsigned char>(1u << bit);
			const bool inter_changed = (inter_byte & mask) != (first_byte & mask);
			const bool winner_unchanged = (winner_byte & mask) == (first_byte & mask);
			const bool current_unclaimed = (current_byte & mask) == (first_byte & mask);

			if (inter_changed && winner_unchanged && current_unclaimed)
				merged = static_cast<unsigned char>((merged & ~mask) | (inter_byte & mask));
		}

		result[offset] = static_cast<char>(merged);
	}
}

static void merge_bool_bit(std::string & result, const sub_record_merge_t::field_merge_input_t & input, const field_def_t & field)
{
	const size_t offset = field.offset;
	if (offset >= input.size)
		return;

	const int bit = static_cast<int>(field.size);
	const unsigned char mask = static_cast<unsigned char>(1u << bit);
	const unsigned char first_byte = static_cast<unsigned char>(input.first[offset]);
	const unsigned char inter_byte = static_cast<unsigned char>(input.inter[offset]);
	const unsigned char winner_byte = static_cast<unsigned char>(input.winner[offset]);
	const unsigned char current_byte = static_cast<unsigned char>(input.current[offset]);

	const bool inter_changed = (inter_byte & mask) != (first_byte & mask);
	const bool winner_unchanged = (winner_byte & mask) == (first_byte & mask);
	const bool current_unclaimed = (current_byte & mask) == (first_byte & mask);

	if (inter_changed && winner_unchanged && current_unclaimed)
		result[offset] = static_cast<char>((current_byte & ~mask) | (inter_byte & mask));
}

static size_t field_span(const field_def_t & field, size_t size)
{
	if (field.size == 0)
		return size > field.offset ? size - field.offset : 0;

	return field.size;
}

static bool span_differs(const char * lhs, const char * rhs, size_t offset, size_t length)
{
	return std::memcmp(lhs + offset, rhs + offset, length) != 0;
}

static void merge_value_field(std::string & result, const sub_record_merge_t::field_merge_input_t & input, const field_def_t & field)
{
	const size_t length = field_span(field, input.size);
	if (length == 0 || field.offset + length > input.size)
		return;

	const bool inter_changed = span_differs(input.inter, input.first, field.offset, length);
	const bool winner_unchanged = !span_differs(input.winner, input.first, field.offset, length);
	const bool current_unclaimed = !span_differs(input.current, input.first, field.offset, length);

	if (inter_changed && winner_unchanged && current_unclaimed)
		std::memcpy(result.data() + field.offset, input.inter + field.offset, length);
}

bool sub_record_merge_t::has_schema(const std::string & rec_type, const std::string & sub_type)
{
	return find_largest_schema(rec_type, sub_type) != nullptr;
}

std::string sub_record_merge_t::merge_fields_three_way(const field_merge_input_t & input)
{
	std::string result(input.current, input.size);

	const auto * schema = find_schema(input.rec_type, input.sub_type, input.size);
	if (!schema)
	{
		app_logger_t::add_log("[error] no schema for " + input.rec_type + ":" + input.sub_type + "\r\n", true);
		return result;
	}

	for (size_t field_index = 0; field_index < schema->field_count; ++field_index)
	{
		const auto & field = schema->fields[field_index];

		if (is_flags_field(field))
		{
			merge_field_bits(result, input, field);
			continue;
		}

		if (field.type == field_type_t::bool_bit)
		{
			merge_bool_bit(result, input, field);
			continue;
		}

		merge_value_field(result, input, field);
	}

	return result;
}

std::vector<sub_record_merge_t::keyed_item_t> sub_record_merge_t::keyed_list_merge(
    const std::vector<std::vector<keyed_item_t>> & versions)
{
	if (versions.empty())
		return {};

	const auto & master = versions.front();

	std::map<std::string, std::string> master_data;
	for (const auto & item : master)
		master_data.emplace(item.key, item.data);

	auto find_in_version = [](const std::vector<keyed_item_t> & version, const std::string & key) -> const keyed_item_t *
	{
		for (const auto & item : version)
		{
			if (item.key == key)
				return &item;
		}

		return nullptr;
	};

	auto resolve_master_item = [&](const std::string & key) -> std::optional<std::string>
	{
		std::optional<std::string> decision = master_data.at(key);

		for (size_t version_index = 1; version_index < versions.size(); ++version_index)
		{
			const auto * present = find_in_version(versions[version_index], key);

			if (present == nullptr)
			{
				decision = std::nullopt;
				continue;
			}

			if (present->data != master_data.at(key))
				decision = present->data;
		}

		return decision;
	};

	auto highest_addition_data = [&](const std::string & key) -> std::string
	{
		for (size_t version_index = versions.size(); version_index-- > 1;)
		{
			const auto * present = find_in_version(versions[version_index], key);
			if (present != nullptr)
				return present->data;
		}

		return {};
	};

	std::vector<keyed_item_t> result;

	for (const auto & item : master)
	{
		const auto decision = resolve_master_item(item.key);
		if (decision.has_value())
			result.push_back({ item.key, *decision });
	}

	std::set<std::string> emitted_keys;
	for (const auto & item : result)
		emitted_keys.insert(item.key);

	for (const auto & version : versions)
	{
		for (const auto & item : version)
		{
			if (master_data.count(item.key) > 0 || emitted_keys.count(item.key) > 0)
				continue;

			result.push_back({ item.key, highest_addition_data(item.key) });
			emitted_keys.insert(item.key);
		}
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

static bool field_changed(const std::string & source, const std::string & base, size_t offset, size_t length)
{
	return source.compare(offset, length, base, offset, length) != 0;
}

static void fix_paired_fields(
    std::string & merged,
    const std::string & first,
    const std::string & inter,
    const std::string & winner,
    const field_pair_rule_t * pairs,
    size_t pair_count)
{
	for (size_t pair_idx = 0; pair_idx < pair_count; ++pair_idx)
	{
		const auto & pair = pairs[pair_idx];
		const bool inter_changed_min = field_changed(inter, first, pair.min_offset, pair.field_size);
		const bool inter_changed_max = field_changed(inter, first, pair.max_offset, pair.field_size);
		const bool winner_changed_min = field_changed(winner, first, pair.min_offset, pair.field_size);
		const bool winner_changed_max = field_changed(winner, first, pair.max_offset, pair.field_size);

		if (winner_changed_min || winner_changed_max)
		{
			merged.replace(pair.min_offset, pair.field_size, winner, pair.min_offset, pair.field_size);
			merged.replace(pair.max_offset, pair.field_size, winner, pair.max_offset, pair.field_size);
			continue;
		}

		if (inter_changed_min || inter_changed_max)
		{
			merged.replace(pair.min_offset, pair.field_size, inter, pair.min_offset, pair.field_size);
			merged.replace(pair.max_offset, pair.field_size, inter, pair.max_offset, pair.field_size);
		}
	}
}

static constexpr field_pair_rule_t enam_magnitude_pair = { 12, 16, 4 };

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

static std::string merge_enam_slot_bytes(
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

		result += merge_enam_slot_bytes(
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
	return is_enam_effect_list(rec_type);
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

void sub_record_merge_t::apply_paired_rules(
    std::string & merged_data,
    const std::string & first_data,
    const sub_record_entry_t & intermediate_entry,
    const std::string & winner_data,
    const std::string & rec_type)
{
	const auto * behavior = find_record_behavior(rec_type);
	if (!behavior || !behavior->paired_rules)
		return;

	for (size_t r = 0; r < behavior->paired_rule_count; ++r)
	{
		const auto & paired = behavior->paired_rules[r];
		if (intermediate_entry.type != paired.sub_type)
			continue;

		if (first_data.size() != paired.expected_size)
			continue;

		fix_paired_fields(
		    merged_data, first_data, intermediate_entry.data, winner_data, paired.pairs, paired.pair_count);
	}
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

		if (intermediate[i].type == "NPCS")
			continue;

		if (is_keyed_list_sub_type(rec_type, intermediate[i].type))
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

		const matched_entry_t entries {
			first[static_cast<size_t>(first_idx)],
			intermediate[i],
			winner[static_cast<size_t>(winner_idx)],
			output[static_cast<size_t>(output_idx)]
		};

		merge_matched_entry(entries, rec_type);
	}
}

void sub_record_merge_t::merge_matched_entry(const matched_entry_t & entries, const std::string & rec_type)
{
	const auto & first_data = entries.first_entry.data;
	const auto & inter_data = entries.inter_entry.data;
	const auto & winner_data = entries.winner_entry.data;

	const bool same_size = inter_data.size() == first_data.size() &&
	                       entries.output_entry.data.size() == first_data.size() &&
	                       winner_data.size() == first_data.size();

	if (same_size && has_schema(rec_type, entries.inter_entry.type))
	{
		const field_merge_input_t input {
			rec_type,
			entries.inter_entry.type,
			first_data.data(),
			inter_data.data(),
			winner_data.data(),
			entries.output_entry.data.data(),
			first_data.size()
		};

		entries.output_entry.data = merge_fields_three_way(input);
		apply_paired_rules(entries.output_entry.data, first_data, entries.inter_entry, winner_data, rec_type);

		return;
	}

	if (winner_data != first_data)
		return;

	if (entries.output_entry.data != first_data)
		return;

	entries.output_entry.data = inter_data;
}

merge_result_t sub_record_merge_t::merge(const merge_input_t & input)
{
	switch (merge_strategy_for(input.rec_type))
	{
	case merge_strategy_t::cell_refs:
		return merge_cell_refs(input);

	case merge_strategy_t::armor_parts:
		return merge_armor_parts(input);

	case merge_strategy_t::no_merge:
		return { false, input.version_contents.back() };

	case merge_strategy_t::generic:
		return merge_generic(input);
	}

	return merge_generic(input);
}

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

		const matched_entry_t entries {
			first[static_cast<size_t>(first_idx)],
			intermediate[i],
			winner[static_cast<size_t>(winner_idx)],
			output[static_cast<size_t>(output_idx)]
		};

		merge_matched_entry(entries, "CELL");
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
	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_part = partition_cell(versions[version_idx]);

		for (const auto & group : inter_part.groups)
		{
			if (first_map.count(group.frmr_index) > 0)
				continue;

			if (winner_map.count(group.frmr_index) > 0)
				continue;

			const bool already_added = std::any_of(
			    merged_groups.begin(),
			    merged_groups.end(),
			    [&](const frmr_group_t & existing) { return existing.frmr_index == group.frmr_index; });

			if (!already_added)
				merged_groups.push_back(group);
		}
	}
}

static bool cell_refs_are_atomic()
{
	const auto * behavior = find_record_behavior("CELL");
	return behavior != nullptr && behavior->atomic_groups;
}

static sub_record_sequence_t select_atomic_frmr_subs(
    const std::vector<std::string> & versions,
    uint32_t index,
    const sub_record_sequence_t & first_subs,
    const sub_record_sequence_t & winner_subs)
{
	if (winner_subs != first_subs)
		return winner_subs;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_part = sub_record_merge_t::partition_cell(versions[version_idx]);
		const auto inter_map = sub_record_merge_t::build_frmr_map(inter_part.groups);
		const auto it_inter = inter_map.find(index);

		if (it_inter == inter_map.end())
			continue;

		if (it_inter->second.sub_records != first_subs)
			return it_inter->second.sub_records;
	}

	return winner_subs;
}

void sub_record_merge_t::merge_winner_frmr_groups(
    std::vector<frmr_group_t> & merged_groups,
    const std::vector<std::string> & versions,
    const frmr_map_t & first_map,
    const frmr_map_t & winner_map)
{
	const bool atomic = cell_refs_are_atomic();

	for (const auto & [index, winner_group] : winner_map)
	{
		auto it_first = first_map.find(index);

		if (it_first == first_map.end())
		{
			merged_groups.push_back(winner_group);
			continue;
		}

		if (atomic)
		{
			auto atomic_subs = select_atomic_frmr_subs(versions, index, it_first->second.sub_records, winner_group.sub_records);
			merged_groups.push_back({ index, std::move(atomic_subs) });
			continue;
		}

		auto merged_subs = winner_group.sub_records;

		for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
		{
			const auto inter_part = partition_cell(versions[version_idx]);
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

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_part = partition_cell(versions[version_idx]);
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

armor_partition_t sub_record_merge_t::partition_armor(const std::string & content)
{
	armor_partition_t result;
	const auto subs = parse_sub_records(content);

	bool in_body_part = false;

	for (const auto & entry : subs)
	{
		const bool starts_body_part = entry.type == "INDX" && entry.data.size() >= 4;

		if (starts_body_part)
		{
			in_body_part = true;
			uint32_t armor_index = 0;
			std::memcpy(&armor_index, entry.data.data(), 4);

			armor_part_group_t group;
			group.armor_index = armor_index;
			group.sub_records.push_back(entry);
			result.groups.push_back(std::move(group));

			continue;
		}

		const bool part_member = in_body_part && (entry.type == "BNAM" || entry.type == "CNAM");

		if (part_member)
		{
			result.groups.back().sub_records.push_back(entry);

			continue;
		}

		in_body_part = false;
		result.header.push_back(entry);
	}

	return result;
}

armor_part_map_t sub_record_merge_t::build_armor_part_map(const std::vector<armor_part_group_t> & groups)
{
	armor_part_map_t result;

	for (const auto & group : groups)
		result.emplace(group.armor_index, group);

	return result;
}

std::string sub_record_merge_t::reconstruct_armor(
    const std::string & winner_content,
    const sub_record_sequence_t & header,
    const std::vector<armor_part_group_t> & groups)
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

void sub_record_merge_t::merge_winner_armor_groups(
    std::vector<armor_part_group_t> & merged_groups,
    const std::vector<std::string> & versions,
    const armor_part_map_t & first_map,
    const armor_part_map_t & winner_map)
{
	for (const auto & [armor_index, winner_group] : winner_map)
	{
		auto it_first = first_map.find(armor_index);

		if (it_first == first_map.end())
		{
			merged_groups.push_back(winner_group);

			continue;
		}

		auto merged_subs = winner_group.sub_records;

		for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
		{
			const auto inter_part = partition_armor(versions[version_idx]);
			const auto inter_map = build_armor_part_map(inter_part.groups);
			auto it_inter = inter_map.find(armor_index);

			if (it_inter == inter_map.end())
				continue;

			apply_intermediate(
			    merged_subs,
			    it_first->second.sub_records,
			    it_inter->second.sub_records,
			    winner_group.sub_records,
			    "ARMO");
		}

		merged_groups.push_back({ armor_index, std::move(merged_subs) });
	}
}

void sub_record_merge_t::collect_intermediate_armor_additions(
    std::vector<armor_part_group_t> & merged_groups,
    const std::vector<std::string> & versions,
    const armor_part_map_t & first_map,
    const armor_part_map_t & winner_map)
{
	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_part = partition_armor(versions[version_idx]);

		for (const auto & group : inter_part.groups)
		{
			if (first_map.count(group.armor_index) > 0)
				continue;

			if (winner_map.count(group.armor_index) > 0)
				continue;

			const bool already_added = std::any_of(
			    merged_groups.begin(),
			    merged_groups.end(),
			    [&](const armor_part_group_t & existing) { return existing.armor_index == group.armor_index; });

			if (!already_added)
				merged_groups.push_back(group);
		}
	}
}

merge_result_t sub_record_merge_t::merge_armor_parts(const merge_input_t & input)
{
	const auto & versions = input.version_contents;

	if (versions.size() < 3)
		return { false, versions.back() };

	const auto & first_content = versions.front();
	const auto & winner_content = versions.back();

	const auto first_part = partition_armor(first_content);
	const auto winner_part = partition_armor(winner_content);

	auto merged_header = winner_part.header;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_part = partition_armor(versions[version_idx]);
		apply_intermediate(merged_header, first_part.header, inter_part.header, winner_part.header, input.rec_type);
	}

	const auto first_map = build_armor_part_map(first_part.groups);
	const auto winner_map = build_armor_part_map(winner_part.groups);

	std::vector<armor_part_group_t> merged_groups;
	merge_winner_armor_groups(merged_groups, versions, first_map, winner_map);
	collect_intermediate_armor_additions(merged_groups, versions, first_map, winner_map);

	std::sort(
	    merged_groups.begin(),
	    merged_groups.end(),
	    [](const armor_part_group_t & lhs, const armor_part_group_t & rhs)
	{ return lhs.armor_index < rhs.armor_index; });

	const auto result = reconstruct_armor(winner_content, merged_header, merged_groups);

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

static std::string extract_npcs_spell_id(const sub_record_entry_t & entry)
{
	auto spell_id = entry.data;
	auto null_pos = spell_id.find('\0');
	if (null_pos != std::string::npos)
		spell_id.resize(null_pos);

	return spell_id;
}

static std::string strip_trailing_null(const std::string & value)
{
	auto result = value;
	auto null_pos = result.find('\0');
	if (null_pos != std::string::npos)
		result.resize(null_pos);

	return result;
}

static std::vector<sub_record_merge_t::keyed_item_t> collect_faction_reactions(const sub_record_sequence_t & sequence)
{
	std::map<std::string, std::string> lowest_by_faction;
	std::vector<std::string> order;

	for (size_t i = 0; i + 1 < sequence.size(); ++i)
	{
		if (sequence[i].type != "ANAM" || sequence[i + 1].type != "INTV")
			continue;

		const auto faction = strip_trailing_null(sequence[i].data);
		const auto & value = sequence[i + 1].data;

		auto it_existing = lowest_by_faction.find(faction);
		if (it_existing == lowest_by_faction.end())
		{
			lowest_by_faction.emplace(faction, value);
			order.push_back(faction);
			continue;
		}

		int32_t existing_value = 0;
		int32_t new_value = 0;
		std::memcpy(&existing_value, it_existing->second.data(), std::min<size_t>(4, it_existing->second.size()));
		std::memcpy(&new_value, value.data(), std::min<size_t>(4, value.size()));

		if (new_value < existing_value)
			it_existing->second = value;
	}

	std::vector<sub_record_merge_t::keyed_item_t> result;
	for (const auto & faction : order)
		result.push_back({ faction, lowest_by_faction.at(faction) });

	return result;
}

static sub_record_sequence_t replace_faction_reactions(
    const sub_record_sequence_t & output,
    const std::vector<sub_record_merge_t::keyed_item_t> & merged_reactions)
{
	sub_record_sequence_t result;

	for (const auto & entry : output)
	{
		if (entry.type != "ANAM" && entry.type != "INTV")
			result.push_back(entry);
	}

	for (const auto & reaction : merged_reactions)
	{
		result.push_back({ "ANAM", reaction.key + '\0' });
		result.push_back({ "INTV", reaction.data });
	}

	return result;
}

static std::vector<sub_record_entry_t> collect_entries_of_type(
    const sub_record_sequence_t & sequence,
    const std::string & sub_type)
{
	std::vector<sub_record_entry_t> result;

	for (const auto & entry : sequence)
	{
		if (entry.type == sub_type)
			result.push_back(entry);
	}

	return result;
}

static bool has_entries_of_type(const sub_record_sequence_t & sequence, const std::string & sub_type)
{
	for (const auto & entry : sequence)
	{
		if (entry.type == sub_type)
			return true;
	}

	return false;
}

static sub_record_sequence_t replace_entries_of_type(
    const sub_record_sequence_t & output,
    const std::vector<sub_record_entry_t> & merged_items,
    const std::string & sub_type)
{
	sub_record_sequence_t result;

	for (const auto & entry : output)
	{
		if (entry.type != sub_type)
			result.push_back(entry);
	}

	for (const auto & item : merged_items)
		result.push_back(item);

	return result;
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

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_subs = parse_sub_records(versions[version_idx]);
		apply_intermediate(output, first_subs, inter_subs, winner_subs, input.rec_type);
	}

	if (is_enam_record_type(input.rec_type))
		output = merge_enam_phase(versions, first_subs, winner_subs, output);

	if (has_entries_of_type(first_subs, "NPCO"))
		output = merge_keyed_list_phase(input, first_subs, winner_subs, output, "NPCO", extract_npco_item_id);

	if (has_entries_of_type(first_subs, "NPCS"))
		output = merge_keyed_list_phase(input, first_subs, winner_subs, output, "NPCS", extract_npcs_spell_id);

	if (decode_mode_for(input.rec_type) == decode_mode_t::faction)
	{
		std::vector<std::vector<keyed_item_t>> reaction_versions;
		reaction_versions.push_back(collect_faction_reactions(first_subs));
		for (size_t version_idx = 1; version_idx < versions.size(); ++version_idx)
			reaction_versions.push_back(collect_faction_reactions(parse_sub_records(versions[version_idx])));

		const auto merged_reactions = keyed_list_merge(reaction_versions);
		output = replace_faction_reactions(output, merged_reactions);
	}

	const auto result = reconstruct_record(winner_content, output);

	if (result == winner_content)
		return { false, winner_content };

	return { true, result };
}

sub_record_sequence_t sub_record_merge_t::merge_enam_phase(
    const std::vector<std::string> & versions,
    const sub_record_sequence_t & first_subs,
    const sub_record_sequence_t & winner_subs,
    const sub_record_sequence_t & output)
{
	const auto first_enams = collect_enam_data(first_subs);
	std::string merged_enam_data;

	for (const auto & slot : collect_enam_data(winner_subs))
		merged_enam_data += slot;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_subs = parse_sub_records(versions[version_idx]);
		const auto inter_enams = collect_enam_data(inter_subs);

		std::vector<std::string> current_slots;
		for (size_t offset = 0; offset + enam_slot_size <= merged_enam_data.size(); offset += enam_slot_size)
			current_slots.push_back(merged_enam_data.substr(offset, enam_slot_size));

		merged_enam_data = merge_enam_slots(first_enams, inter_enams, current_slots);
	}

	return replace_enam_entries(output, merged_enam_data);
}

sub_record_sequence_t sub_record_merge_t::merge_keyed_list_phase(
    const merge_input_t & input,
    const sub_record_sequence_t & first_subs,
    const sub_record_sequence_t & winner_subs,
    const sub_record_sequence_t & output,
    const std::string & sub_type,
    const std::function<std::string(const sub_record_entry_t &)> & key_of)
{
	const auto & versions = input.version_contents;
	const auto winner_items = collect_entries_of_type(winner_subs, sub_type);

	size_t first_contributing = 1;
	for (size_t version_idx = versions.size() - 1; version_idx >= 1; --version_idx)
	{
		if (input.patch_version_indices.count(version_idx))
		{
			first_contributing = version_idx;
			break;
		}
	}

	auto to_keyed = [&key_of](const std::vector<sub_record_entry_t> & items)
	{
		std::vector<keyed_item_t> result;
		for (const auto & item : items)
			result.push_back({ key_of(item), item.data });

		return result;
	};

	std::vector<std::vector<keyed_item_t>> keyed_versions;
	keyed_versions.push_back(to_keyed(collect_entries_of_type(first_subs, sub_type)));

	for (size_t version_idx = first_contributing; version_idx < versions.size(); ++version_idx)
	{
		const auto version_subs = parse_sub_records(versions[version_idx]);
		keyed_versions.push_back(to_keyed(collect_entries_of_type(version_subs, sub_type)));
	}

	const auto merged_keyed = keyed_list_merge(keyed_versions);

	std::vector<sub_record_entry_t> merged_items;
	for (const auto & item : merged_keyed)
		merged_items.push_back({ sub_type, item.data });

	if (merged_items != winner_items)
		return replace_entries_of_type(output, merged_items, sub_type);

	return output;
}

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

static sub_record_sequence_t extract_header_subs(const std::string & content)
{
	sub_record_sequence_t header;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;

	while (iter.next(sub))
	{
		if (sub.type == "INAM" || sub.type == "CNAM" || sub.type == "INTV")
			break;

		if (sub.type == "INDX")
			continue;

		header.push_back({ sub.type, std::string(sub.data, sub.size) });
	}

	return header;
}

static std::string merge_header_part(const std::vector<std::string> & versions, const std::string & rec_type)
{
	const auto first_header = extract_header_subs(versions.front());
	const auto winner_header = extract_header_subs(versions.back());
	auto output = winner_header;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_header = extract_header_subs(versions[version_idx]);
		sub_record_merge_t::apply_intermediate(output, first_header, inter_header, winner_header, rec_type);
	}

	std::string header_part;
	for (const auto & entry : output)
		header_part += sub_record_merge_t::serialize_sub_record(entry);

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

	const char * const item_sub_type_name = leveled_item_sub_type_for(rec_type);
	if (item_sub_type_name == nullptr)
	{
		app_logger_t::add_log("[error] no leveled item sub-type for " + rec_type + "\r\n", true);
		return {};
	}

	const std::string item_sub_type = item_sub_type_name;
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

using item_levels_map_t = std::map<std::string, std::vector<uint16_t>>;

static item_levels_map_t build_item_levels_map(const std::vector<list_item_t> & items)
{
	item_levels_map_t levels;
	for (const auto & item : items)
		levels[item.ident].push_back(item.level);

	return levels;
}

static bool is_item_deleted(
    const std::string & ident,
    const item_levels_map_t & first_map,
    const std::vector<item_levels_map_t> & plugin_maps)
{
	const auto it_first = first_map.find(ident);
	if (it_first == first_map.end() || it_first->second.empty())
		return false;

	for (const auto & version_map : plugin_maps)
	{
		if (version_map.find(ident) == version_map.end())
			return true;
	}

	return false;
}

static size_t version_count(const item_levels_map_t & map, const std::string & ident)
{
	const auto it_map = map.find(ident);
	return it_map == map.end() ? 0 : it_map->second.size();
}

struct count_context_t
{
	const std::string & ident;
	const item_levels_map_t & first_map;
	const std::vector<item_levels_map_t> & plugin_maps;
};

static size_t merged_occurrence_count(const count_context_t & context)
{
	const size_t master_count = version_count(context.first_map, context.ident);

	size_t resolved = master_count;
	for (const auto & version_map : context.plugin_maps)
	{
		const size_t plugin_count = version_count(version_map, context.ident);
		if (plugin_count != master_count)
			resolved = plugin_count;
	}

	return resolved;
}

struct occurrence_context_t
{
	const std::string & ident;
	size_t occurrence;
	const item_levels_map_t & first_map;
	const std::vector<item_levels_map_t> & plugin_maps;
};

static std::optional<uint16_t> level_at(const item_levels_map_t & map, const std::string & ident, size_t occurrence)
{
	const auto it_map = map.find(ident);
	if (it_map == map.end() || occurrence >= it_map->second.size())
		return std::nullopt;

	return it_map->second[occurrence];
}

static uint16_t resolve_occurrence_level(const occurrence_context_t & context)
{
	const auto master_level = level_at(context.first_map, context.ident, context.occurrence);

	uint16_t resolved = master_level.value_or(0);
	bool has_resolved = master_level.has_value();

	for (const auto & version_map : context.plugin_maps)
	{
		const auto plugin_level = level_at(version_map, context.ident, context.occurrence);
		if (!plugin_level.has_value())
			continue;

		if (!master_level.has_value() || *plugin_level != *master_level)
		{
			resolved = *plugin_level;
			has_resolved = true;
		}
	}

	return has_resolved ? resolved : master_level.value_or(0);
}

static std::vector<list_item_t> build_merged_items(
    const item_levels_map_t & first_map,
    const std::vector<item_levels_map_t> & plugin_maps)
{
	std::set<std::string> all_idents;
	for (const auto & [ident, levels] : first_map)
		all_idents.insert(ident);

	for (const auto & version_map : plugin_maps)
	{
		for (const auto & [ident, levels] : version_map)
			all_idents.insert(ident);
	}

	std::vector<list_item_t> merged;
	for (const auto & ident : all_idents)
	{
		if (is_item_deleted(ident, first_map, plugin_maps))
			continue;

		const count_context_t count_context { ident, first_map, plugin_maps };
		const auto count = merged_occurrence_count(count_context);
		for (size_t occurrence = 0; occurrence < count; ++occurrence)
		{
			const occurrence_context_t context { ident, occurrence, first_map, plugin_maps };
			merged.push_back({ ident, resolve_occurrence_level(context) });
		}
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
	auto master_items = extract_list_items(first_content);
	const auto first_map = build_item_levels_map(master_items);

	std::vector<item_levels_map_t> plugin_maps;

	for (size_t vi = 1; vi < versions.size(); ++vi)
		plugin_maps.push_back(build_item_levels_map(extract_list_items(versions[vi])));

	if (plugin_maps.empty())
		return { false, {} };

	auto merged = build_merged_items(first_map, plugin_maps);
	sort_merged_items(merged);

	const auto header_part = merge_header_part(versions, input.rec_type);
	const auto record = build_merged_list_record(input.rec_type, header_part, merged);

	sort_merged_items(master_items);
	const auto master_header_part = merge_header_part({ first_content, first_content }, input.rec_type);
	const auto master_record = build_merged_list_record(input.rec_type, master_header_part, master_items);

	if (record == master_record)
		return { false, first_content };

	return { true, record };
}
