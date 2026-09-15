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

static constexpr size_t record_header_size = record_layout::header_size;
static constexpr size_t record_size_field_offset = record_layout::size_field_offset;
static constexpr size_t record_size_field_length = record_layout::size_field_length;
static constexpr size_t object_index_size = object_index_layout::index_size;
static constexpr size_t leveled_level_size = leveled_layout::level_size;

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

	std::string result = winner_content.substr(0, record_header_size);
	const auto body_size = domain_types::convert_uint_to_string_byte_array(body.size());
	result.replace(record_size_field_offset, record_size_field_length, body_size);
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

static bool intermediate_claims_bit(
    unsigned char first_byte,
    unsigned char inter_byte,
    unsigned char winner_byte,
    unsigned char current_byte,
    unsigned char mask)
{
	const bool inter_changed = (inter_byte & mask) != (first_byte & mask);
	const bool winner_unchanged = (winner_byte & mask) == (first_byte & mask);
	const bool current_unclaimed = (current_byte & mask) == (first_byte & mask);

	return inter_changed && winner_unchanged && current_unclaimed;
}

static bool span_differs(const char * lhs, const char * rhs, size_t offset, size_t length)
{
	return std::memcmp(lhs + offset, rhs + offset, length) != 0;
}

static bool intermediate_claims_span(
    const sub_record_merge_t::field_merge_input_t & input,
    size_t offset,
    size_t length)
{
	const bool inter_changed = span_differs(input.inter, input.first, offset, length);
	const bool winner_unchanged = !span_differs(input.winner, input.first, offset, length);
	const bool current_unclaimed = !span_differs(input.current, input.first, offset, length);

	return inter_changed && winner_unchanged && current_unclaimed;
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
			if (intermediate_claims_bit(first_byte, inter_byte, winner_byte, current_byte, mask))
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

	if (intermediate_claims_bit(first_byte, inter_byte, winner_byte, current_byte, mask))
		result[offset] = static_cast<char>((current_byte & ~mask) | (inter_byte & mask));
}

static size_t field_span(const field_def_t & field, size_t size)
{
	if (field.size == 0)
		return size > field.offset ? size - field.offset : 0;

	return field.size;
}

static void merge_value_field(std::string & result, const sub_record_merge_t::field_merge_input_t & input, const field_def_t & field)
{
	const size_t length = field_span(field, input.size);
	if (length == 0 || field.offset + length > input.size)
		return;

	if (intermediate_claims_span(input, field.offset, length))
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

static bool is_keyed_list_spec_sub_type(const std::string & rec_type, const std::string & sub_type);

void sub_record_merge_t::apply_intermediate(
    sub_record_sequence_t & output,
    const sub_record_sequence_t & first,
    const sub_record_sequence_t & intermediate,
    const sub_record_sequence_t & winner,
    const std::string & rec_type)
{
	for (size_t i = 0; i < intermediate.size(); ++i)
	{
		if (is_keyed_list_spec_sub_type(rec_type, intermediate[i].type))
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
	return merge_generic(input);
}

uint32_t sub_record_merge_t::read_frmr_index(const sub_record_entry_t & frmr_entry)
{
	return static_cast<uint32_t>(
	    domain_types::convert_string_byte_array_to_uint(frmr_entry.data.substr(0, object_index_size)));
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

static std::string extract_keyed_list_key(const sub_record_entry_t & entry, const keyed_list_spec_t & spec)
{
	if (entry.data.size() < spec.minimum_record_size)
		return {};

	if (spec.key_source == key_source_t::identity_prefix || spec.key_source == key_source_t::index_value)
		return spec.key_length == 0 ? entry.data : entry.data.substr(spec.key_offset, spec.key_length);

	auto key = spec.key_length == 0 ? entry.data : entry.data.substr(spec.key_offset, spec.key_length);

	auto null_pos = key.find('\0');
	if (null_pos != std::string::npos)
		key.resize(null_pos);

	return key;
}

static bool is_group_member(const keyed_list_spec_t & spec, const std::string & sub_type)
{
	for (size_t member_idx = 0; member_idx < spec.member_sub_type_count; ++member_idx)
	{
		if (sub_type == spec.member_sub_types[member_idx])
			return true;
	}

	return false;
}

static std::vector<sub_record_merge_t::keyed_item_t> collect_grouped_units(
    const sub_record_sequence_t & sequence,
    const keyed_list_spec_t & spec)
{
	std::vector<sub_record_merge_t::keyed_item_t> units;
	std::string current_key;
	std::string current_blob;
	bool in_unit = false;

	auto flush = [&units, &current_key, &current_blob, &in_unit]()
	{
		if (in_unit)
			units.push_back({ current_key, current_blob });
	};

	for (const auto & entry : sequence)
	{
		if (entry.type == spec.sub_type)
		{
			flush();
			current_key = extract_keyed_list_key(entry, spec);
			current_blob = sub_record_merge_t::serialize_sub_record(entry);
			in_unit = true;

			continue;
		}

		if (in_unit && is_group_member(spec, entry.type))
			current_blob += sub_record_merge_t::serialize_sub_record(entry);
	}

	flush();

	return units;
}

static bool is_keyed_list_spec_sub_type(const std::string & rec_type, const std::string & sub_type)
{
	size_t spec_count = 0;
	const auto * specs = keyed_list_specs_for(rec_type, spec_count);

	for (size_t spec_idx = 0; spec_idx < spec_count; ++spec_idx)
	{
		const auto & spec = specs[spec_idx];
		if (sub_type == spec.sub_type)
			return true;

		for (size_t member_idx = 0; member_idx < spec.member_sub_type_count; ++member_idx)
		{
			if (sub_type == spec.member_sub_types[member_idx])
				return true;
		}
	}

	return false;
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

static sub_record_sequence_t replace_grouped_units(
    const sub_record_sequence_t & output,
    const std::vector<sub_record_merge_t::keyed_item_t> & merged_units,
    const keyed_list_spec_t & spec)
{
	sub_record_sequence_t result;

	for (const auto & entry : output)
	{
		if (entry.type == spec.sub_type || is_group_member(spec, entry.type))
			continue;

		result.push_back(entry);
	}

	for (const auto & unit : merged_units)
	{
		for (const auto & entry : sub_record_merge_t::parse_sub_records(unit.data))
			result.push_back(entry);
	}

	return result;
}

static sub_record_sequence_t filter_for_merge(
    const sub_record_sequence_t & sequence,
    const record_behavior_t * behavior)
{
	sub_record_sequence_t result;

	for (const auto & entry : sequence)
	{
		const auto * rule = find_sub_record_rule(behavior, entry.type, entry.data.size());

		if (rule && has_flag(rule->flags, sub_rule_flag_t::merge_boundary))
			break;

		if (rule && has_flag(rule->flags, sub_rule_flag_t::skip_emit))
			continue;

		result.push_back(entry);
	}

	return result;
}

merge_result_t sub_record_merge_t::merge_generic(const merge_input_t & input)
{
	const auto & versions = input.version_contents;

	if (versions.size() < 3)
		return { false, versions.back() };

	const auto & first_content = versions.front();
	const auto & winner_content = versions.back();

	const auto * behavior = find_record_behavior(input.rec_type);

	const auto first_subs = filter_for_merge(parse_sub_records(first_content), behavior);
	const auto winner_subs = filter_for_merge(parse_sub_records(winner_content), behavior);
	auto output = winner_subs;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_subs = filter_for_merge(parse_sub_records(versions[version_idx]), behavior);
		apply_intermediate(output, first_subs, inter_subs, winner_subs, input.rec_type);
	}

	size_t keyed_list_spec_count = 0;
	const auto * keyed_list_specs = keyed_list_specs_for(input.rec_type, keyed_list_spec_count);
	for (size_t spec_idx = 0; spec_idx < keyed_list_spec_count; ++spec_idx)
		output = merge_keyed_list_phase(input, first_subs, winner_subs, output, keyed_list_specs[spec_idx]);

	if (output == winner_subs)
		return { false, winner_content };

	const auto result = reconstruct_record(winner_content, output);
	return { true, result };
}

sub_record_sequence_t sub_record_merge_t::merge_keyed_list_phase(
    const merge_input_t & input,
    const sub_record_sequence_t & first_subs,
    const sub_record_sequence_t & winner_subs,
    const sub_record_sequence_t & output,
    const keyed_list_spec_t & spec)
{
	const auto & versions = input.version_contents;

	size_t first_contributing = 1;
	for (size_t version_idx = versions.size() - 1; version_idx >= 1; --version_idx)
	{
		if (input.patch_version_indices.count(version_idx))
		{
			first_contributing = version_idx;
			break;
		}
	}

	if (spec.member_sub_type_count > 0)
		return merge_grouped_list(input, first_subs, winner_subs, output, spec, first_contributing);

	const std::string sub_type = spec.sub_type;
	const auto key_of = [&spec](const sub_record_entry_t & entry) { return extract_keyed_list_key(entry, spec); };
	const auto winner_items = collect_entries_of_type(winner_subs, sub_type);

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

sub_record_sequence_t sub_record_merge_t::merge_grouped_list(
    const merge_input_t & input,
    const sub_record_sequence_t & first_subs,
    const sub_record_sequence_t & winner_subs,
    const sub_record_sequence_t & output,
    const keyed_list_spec_t & spec,
    size_t first_contributing)
{
	const auto & versions = input.version_contents;

	std::vector<std::vector<keyed_item_t>> keyed_versions;
	keyed_versions.push_back(collect_grouped_units(first_subs, spec));

	for (size_t version_idx = first_contributing; version_idx < versions.size(); ++version_idx)
	{
		const auto version_subs = parse_sub_records(versions[version_idx]);
		keyed_versions.push_back(collect_grouped_units(version_subs, spec));
	}

	const auto merged_units = keyed_list_merge(keyed_versions);

	if (merged_units == collect_grouped_units(winner_subs, spec))
		return output;

	return replace_grouped_units(output, merged_units, spec);
}

struct list_item_t
{
	std::string ident;
	uint16_t level;
};

static std::vector<list_item_t> extract_list_items(const std::string & content, const std::string & rec_type)
{
	std::vector<list_item_t> items;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	std::string current_id;

	const std::string item_sub_type = leveled_item_sub_type_for(rec_type);
	const std::string level_sub_type = leveled_level_sub_type_for(rec_type);

	while (iter.next(sub))
	{
		if (sub.type == item_sub_type)
		{
			current_id = std::string(sub.data, sub.size);
			current_id = string_utils::erase_null_chars(current_id);
			continue;
		}

		if (sub.type == level_sub_type && !current_id.empty())
		{
			uint16_t level = 0;
			if (sub.size >= leveled_level_size)
				std::memcpy(&level, sub.data, leveled_level_size);

			items.push_back({ current_id, level });
			current_id.clear();
		}
	}

	return items;
}

static sub_record_sequence_t extract_header_subs(const std::string & content, const std::string & rec_type)
{
	sub_record_sequence_t header;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;

	const std::string item_sub_type = leveled_item_sub_type_for(rec_type);
	const std::string level_sub_type = leveled_level_sub_type_for(rec_type);

	while (iter.next(sub))
	{
		if (sub.type == item_sub_type || sub.type == level_sub_type)
			break;

		if (sub.type == "INDX")
			continue;

		header.push_back({ sub.type, std::string(sub.data, sub.size) });
	}

	return header;
}

static std::string merge_header_part(const std::vector<std::string> & versions, const std::string & rec_type)
{
	const auto first_header = extract_header_subs(versions.front(), rec_type);
	const auto winner_header = extract_header_subs(versions.back(), rec_type);
	auto output = winner_header;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_header = extract_header_subs(versions[version_idx], rec_type);
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
	const char * const level_sub_type_name = leveled_level_sub_type_for(rec_type);
	if (item_sub_type_name == nullptr || level_sub_type_name == nullptr)
	{
		app_logger_t::add_log("[error] no leveled item sub-type for " + rec_type + "\r\n", true);
		return {};
	}

	const std::string item_sub_type = item_sub_type_name;
	const std::string level_sub_type = level_sub_type_name;
	std::string items_part;
	for (const auto & item : merged_items)
	{
		std::string id_data = item.ident;
		id_data.push_back('\0');

		items_part += item_sub_type;
		items_part += domain_types::convert_uint_to_string_byte_array(id_data.size());
		items_part += id_data;

		items_part += level_sub_type;
		items_part += domain_types::convert_uint_to_string_byte_array(leveled_level_size);
		items_part += std::string(reinterpret_cast<const char *>(&item.level), leveled_level_size);
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
	auto master_items = extract_list_items(first_content, input.rec_type);
	const auto first_map = build_item_levels_map(master_items);

	std::vector<item_levels_map_t> plugin_maps;

	for (size_t vi = 1; vi < versions.size(); ++vi)
		plugin_maps.push_back(build_item_levels_map(extract_list_items(versions[vi], input.rec_type)));

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
