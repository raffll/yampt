#include "view_tree_model.hpp"
#include <decoder/view_tree_format.hpp>
#include <scanner/record_conflict.hpp>
#include <utility/app_logger.hpp>
#include <cstdio>
#include <cstring>

static bool check_all_identical(const std::vector<std::string> & values)
{
	for (size_t col = 1; col < values.size(); ++col)
	{
		if (values[col] != values[0])
			return false;
	}
	return true;
}

static void apply_cell_composition_label(view_tree_model_t::view_node_t & row, const std::string & section_key)
{
	for (const auto & entry : record_composition(section_key))
	{
		if (row.type != entry.sub_type || entry.label == nullptr)
			continue;

		row.label = row.type + " - " + entry.label;
		return;
	}
}

static void propagate_conflict_upward(
    view_tree_model_t::view_node_t & parent,
    const view_tree_model_t::view_node_t & child,
    size_t col_count)
{
	if (child.row_conflict_all > parent.row_conflict_all)
		parent.row_conflict_all = child.row_conflict_all;

	for (size_t col = 0; col < col_count && col < child.cell_conflict_this.size(); ++col)
	{
		if (child.cell_conflict_this[col] > parent.cell_conflict_this[col])
			parent.cell_conflict_this[col] = child.cell_conflict_this[col];
	}
}

static std::string format_hex_chunk(const char * data_ptr, size_t data_size, size_t offset)
{
	if (offset >= data_size)
		return "";

	const size_t chunk = std::min(static_cast<size_t>(16), data_size - offset);
	std::string hex_text;
	for (size_t byte_idx = 0; byte_idx < chunk; ++byte_idx)
	{
		char hbuf[4];
		std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(data_ptr[offset + byte_idx]));
		if (!hex_text.empty())
			hex_text += ' ';

		hex_text += hbuf;
	}
	return hex_text;
}

static uint32_t read_object_index(const sub_record_view_t & sub_rec)
{
	return read_frmr_ref_index(sub_rec.data, sub_rec.size);
}

static std::string read_ref_object_id(
    const std::vector<sub_record_view_t> & subs,
    size_t start_idx,
    size_t end_idx)
{
	for (size_t i = start_idx; i < end_idx; ++i)
	{
		if (subs[i].type != "NAME")
			continue;

		std::string object_id(subs[i].data, subs[i].size);
		if (!object_id.empty() && object_id.back() == '\0')
			object_id.pop_back();

		return object_id;
	}

	return {};
}

static void collect_cell_ref_groups(
    const std::vector<sub_record_view_t> & subs,
    const std::vector<uint64_t> & ref_identities,
    std::vector<cell_ref_view_t> & out_refs,
    size_t & out_header_end)
{
	size_t nam0_pos = SIZE_MAX;
	for (size_t i = 0; i < subs.size(); ++i)
	{
		if (subs[i].type == "NAM0")
		{
			nam0_pos = i;
			break;
		}
	}

	size_t frmr_ordinal = 0;
	for (size_t i = 0; i < subs.size(); ++i)
	{
		if (subs[i].type != "FRMR")
			continue;

		if (out_refs.empty())
		{
			out_header_end = i;
			if (nam0_pos != SIZE_MAX && nam0_pos < i)
				out_header_end = i;
		}

		const auto obj_idx = read_object_index(subs[i]);

		size_t group_end = subs.size();
		for (size_t j = i + 1; j < subs.size(); ++j)
		{
			if (subs[j].type == "FRMR" || subs[j].type == "NAM0")
			{
				group_end = j;
				break;
			}
		}

		if (frmr_ordinal >= ref_identities.size())
		{
			app_logger_t::add_log(
			    "[error] CELL ref identity missing for FRMR ordinal " + std::to_string(frmr_ordinal) + "\r\n");
			++frmr_ordinal;
			continue;
		}

		ref_key_t identity { ref_identities[frmr_ordinal], read_ref_object_id(subs, i, group_end) };
		const bool is_persistent = (nam0_pos == SIZE_MAX) || (i < nam0_pos);
		out_refs.push_back({ obj_idx, identity, i, group_end, is_persistent });
		++frmr_ordinal;
	}

	if (out_refs.empty())
		out_header_end = subs.size();
}

static void collect_unique_ref_identities(
    const std::vector<std::vector<cell_ref_view_t>> & col_refs,
    std::vector<ref_key_t> & all_identities)
{
	for (const auto & refs : col_refs)
	{
		for (const auto & ref_group : refs)
		{
			bool found = false;
			for (const auto & existing : all_identities)
			{
				if (existing == ref_group.identity)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_identities.push_back(ref_group.identity);
		}
	}
}

static void build_cell_header_slots(
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<size_t> & col_header_end,
    std::vector<sub_slot_t> & header_slots)
{
	std::vector<size_t> col_start(col_count, 0);
	content_alignment_t::build_occurrence_from_ranges(all_subs, col_count, col_start, col_header_end, header_slots);
}

static void build_ref_slots_for_object(
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::vector<cell_ref_view_t>> & col_refs,
    const ref_key_t & identity,
    std::vector<sub_slot_t> & ref_slots)
{
	std::vector<size_t> col_start(col_count, 0);
	std::vector<size_t> col_end(col_count, 0);

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= col_refs.size())
			continue;

		for (const auto & ref_group : col_refs[col])
		{
			if (!(ref_group.identity == identity))
				continue;

			col_start[col] = ref_group.start_idx;
			col_end[col] = ref_group.end_idx;
			break;
		}
	}

	content_alignment_t::build_occurrence_from_ranges(all_subs, col_count, col_start, col_end, ref_slots);
}

struct ref_lookup_result_t
{
	sub_record_view_t view;
	int binary_index = -1;
};

static ref_lookup_result_t find_ref_sub_record(
    const std::vector<sub_record_view_t> & subs,
    const std::vector<cell_ref_view_t> & refs,
    const ref_key_t & identity,
    const std::string & slot_type,
    int slot_occurrence)
{
	for (const auto & ref_group : refs)
	{
		if (!(ref_group.identity == identity))
			continue;

		int occur = 0;
		for (size_t i = ref_group.start_idx; i < ref_group.end_idx; ++i)
		{
			if (subs[i].type != slot_type)
				continue;

			if (occur == slot_occurrence)
				return { subs[i], static_cast<int>(i) };

			++occur;
		}
		break;
	}

	return { { "", nullptr, 0, 0 }, -1 };
}

void view_tree_model_t::decode_schema_children_ref(
    view_node_t & parent_row,
    const sub_record_schema_t * schema,
    const char *,
    size_t,
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::vector<cell_ref_view_t>> & col_refs,
    const ref_key_t & identity,
    const sub_slot_t & slot)
{
	static const std::vector<sub_record_view_t> empty_subs;
	static const std::vector<cell_ref_view_t> empty_refs;

	for (size_t field_idx = 0; field_idx < schema->field_count; ++field_idx)
	{
		const auto & fdef = schema->fields[field_idx];
		const bool is_flags =
		    (fdef.type == field_type_t::flags_u8 || fdef.type == field_type_t::flags_u16 ||
		     fdef.type == field_type_t::flags_u32);

		if (is_flags && fdef.flag_names && fdef.flag_count > 0)
		{
			for (int bit = 0; bit < fdef.flag_count; ++bit)
			{
				if (fdef.flag_names[bit][0] == '_')
					continue;

				view_node_t frow;
				frow.label = fdef.flag_names[bit];
				frow.schema_field_index = static_cast<int>(field_idx);
				frow.bit_index = bit;
				frow.values.resize(col_count);

				for (size_t col = 0; col < col_count; ++col)
				{
					const auto & subs = col < all_subs.size() ? all_subs[col] : empty_subs;
					const auto & refs = col < col_refs.size() ? col_refs[col] : empty_refs;
					const auto result = find_ref_sub_record(subs, refs, identity, slot.type, slot.occurrence);

					frow.values[col] =
					    result.view.data ? flag_bit_value(result.view.data, result.view.size, fdef, bit) : non_existent_value;
				}

				frow.all_identical = check_all_identical(frow.values);
				frow.row_conflict_all = record_conflict::compute_conflict_all_skip_empty(frow.values);
				frow.cell_conflict_this = record_conflict::compute_conflict_this_skip_empty(frow.values);
				parent_row.children.push_back(std::move(frow));
			}
			continue;
		}

		view_node_t frow;
		frow.label = fdef.name;
		frow.schema_field_index = static_cast<int>(field_idx);
		frow.values.resize(col_count);

		for (size_t col = 0; col < col_count; ++col)
		{
			const auto & subs = col < all_subs.size() ? all_subs[col] : empty_subs;
			const auto & refs = col < col_refs.size() ? col_refs[col] : empty_refs;
			const auto result = find_ref_sub_record(subs, refs, identity, slot.type, slot.occurrence);

			frow.values[col] = result.view.data
			                       ? decode_field(fdef, result.view.data, result.view.size, m_display_codepage)
			                       : non_existent_value;
		}

		frow.all_identical = check_all_identical(frow.values);
		frow.row_conflict_all = record_conflict::compute_conflict_all_skip_empty(frow.values);
		frow.cell_conflict_this = record_conflict::compute_conflict_this_skip_empty(frow.values);
		parent_row.children.push_back(std::move(frow));
	}
}

void view_tree_model_t::decode_hex_children_ref(
    view_node_t & parent_row,
    size_t first_size,
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::vector<cell_ref_view_t>> & col_refs,
    const ref_key_t & identity,
    const sub_slot_t & slot)
{
	static const std::vector<sub_record_view_t> empty_subs;
	static const std::vector<cell_ref_view_t> empty_refs;
	static constexpr size_t hex_line_bytes = 16;

	for (size_t offset = 0; offset < first_size; offset += hex_line_bytes)
	{
		view_node_t frow;
		char name_buf[16];
		std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
		frow.label = name_buf;
		frow.values.resize(col_count);

		for (size_t col = 0; col < col_count; ++col)
		{
			const auto & subs = col < all_subs.size() ? all_subs[col] : empty_subs;
			const auto & refs = col < col_refs.size() ? col_refs[col] : empty_refs;
			const auto result = find_ref_sub_record(subs, refs, identity, slot.type, slot.occurrence);

			frow.values[col] =
			    result.view.data ? format_hex_chunk(result.view.data, result.view.size, offset) : non_existent_value;
		}

		frow.all_identical = check_all_identical(frow.values);
		frow.row_conflict_all = record_conflict::compute_conflict_all_skip_empty(frow.values);
		frow.cell_conflict_this = record_conflict::compute_conflict_this_skip_empty(frow.values);
		parent_row.children.push_back(std::move(frow));
	}
}

static bool is_ref_persistent(const std::vector<std::vector<cell_ref_view_t>> & col_refs, const ref_key_t & identity)
{
	for (const auto & refs : col_refs)
	{
		for (const auto & ref_group : refs)
		{
			if (!(ref_group.identity == identity))
				continue;

			return ref_group.persistent;
		}
	}

	return false;
}

view_tree_model_t::view_node_t view_tree_model_t::build_ref_child(
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::vector<cell_ref_view_t>> & col_refs,
    const ref_key_t & identity,
    const sub_slot_t & slot)
{
	const char * first_data = nullptr;
	size_t first_size = 0;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto result = find_ref_sub_record(all_subs[col], col_refs[col], identity, slot.type, slot.occurrence);
		if (result.view.data && !first_data)
		{
			first_data = result.view.data;
			first_size = result.view.size;
		}
	}

	const auto * schema = first_data ? find_schema(m_record_type, slot.type, first_size) : nullptr;

	if (schema && schema->field_count > 1)
	{
		view_node_t sub_group;
		sub_group.type = slot.type;
		sub_group.size = 0;
		sub_group.label = make_sub_label(slot.type, m_record_type, first_size);
		apply_cell_composition_label(sub_group, "CELL@ref");
		sub_group.values.resize(col_count);
		sub_group.cell_conflict_this.resize(col_count, conflict_this_t::unknown);
		sub_group.row_conflict_all = conflict_all_t::only_one;

		decode_schema_children_ref(
		    sub_group, schema, first_data, first_size, col_count, all_subs, col_refs, identity, slot);

		for (size_t col = 0; col < col_count; ++col)
		{
			bool present = false;
			if (col < col_refs.size())
			{
				for (const auto & ref_group : col_refs[col])
				{
					if (ref_group.identity == identity)
					{
						present = true;
						break;
					}
				}
			}

			sub_group.values[col] = present ? slot.type : non_existent_value;
		}

		sub_group.all_identical = check_all_identical(sub_group.values);

		for (const auto & child : sub_group.children)
			propagate_conflict_upward(sub_group, child, col_count);

		compute_group_ranges(sub_group, col_count);
		return sub_group;
	}

	view_node_t child_field;
	child_field.type = slot.type;
	child_field.size = first_size;
	child_field.values.resize(col_count);
	child_field.binary_ranges.resize(col_count);

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
		{
			child_field.values[col] = non_existent_value;
			continue;
		}

		const auto result = find_ref_sub_record(all_subs[col], col_refs[col], identity, slot.type, slot.occurrence);
		if (!result.view.data)
		{
			child_field.values[col] = non_existent_value;
			continue;
		}

		child_field.binary_ranges[col] = { result.binary_index, result.binary_index + 1 };
		if (slot.type == "FRMR")
			child_field.values[col] = std::to_string(read_frmr_ref_index(result.view.data, result.view.size));
		else if (slot.type == "DELE")
			child_field.values[col] = "DELETED";
		else if (schema && schema->field_count == 1)
			child_field.values[col] =
			    decode_field(schema->fields[0], result.view.data, result.view.size, m_display_codepage);
		else
			child_field.values[col] = format_value_full(result.view.data, result.view.size, m_display_codepage);
	}

	child_field.label = make_sub_label(slot.type, m_record_type, first_size);
	apply_cell_composition_label(child_field, "CELL@ref");
	child_field.all_identical = check_all_identical(child_field.values);
	child_field.row_conflict_all = record_conflict::compute_conflict_all_skip_empty(child_field.values);
	child_field.cell_conflict_this = record_conflict::compute_conflict_this_skip_empty(child_field.values);

	return child_field;
}

void view_tree_model_t::set_record_cell(record_context_t & context)
{
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<std::vector<cell_ref_view_t>> col_refs(col_count);
	std::vector<size_t> col_header_end(col_count, 0);
	std::vector<ref_key_t> all_ref_identities;

	static const std::vector<uint64_t> empty_identities;
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & identities =
		    (context.slot_result && col < context.slot_result->ref_identities.size())
		        ? context.slot_result->ref_identities[col]
		        : empty_identities;

		collect_cell_ref_groups(all_subs[col], identities, col_refs[col], col_header_end[col]);
	}

	collect_unique_ref_identities(col_refs, all_ref_identities);

	std::vector<sub_slot_t> header_slots;
	build_cell_header_slots(col_count, all_subs, col_header_end, header_slots);

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_header_indices(col_count);
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (size_t i = 0; i < col_header_end[col]; ++i)
			col_header_indices[col][all_subs[col][i].type].push_back(i);
	}

	for (const auto & slot : header_slots)
	{
		if (slot.type == "NAM0")
			continue;

		auto row = build_slot_row(col_count, all_subs, col_header_indices, slot);
		apply_cell_composition_label(row, "CELL");
		m_rows.push_back(std::move(row));
	}

	std::vector<ref_key_t> persistent_identities;
	std::vector<ref_key_t> temporary_identities;

	for (const auto & identity : all_ref_identities)
	{
		if (is_ref_persistent(col_refs, identity))
			persistent_identities.push_back(identity);
		else
			temporary_identities.push_back(identity);
	}

	auto build_ref_group = [&](const ref_key_t & identity) -> view_node_t
	{
		std::vector<sub_slot_t> ref_slots;
		build_ref_slots_for_object(col_count, all_subs, col_refs, identity, ref_slots);

		uint32_t display_index = 0;
		bool display_index_found = false;
		for (size_t col = 0; col < col_count && !display_index_found; ++col)
		{
			if (col >= col_refs.size())
				continue;

			for (const auto & ref_group : col_refs[col])
			{
				if (!(ref_group.identity == identity))
					continue;

				display_index = ref_group.object_index;
				display_index_found = true;
				break;
			}
		}

		const std::string index_label = "#" + std::to_string(display_index);
		std::string ref_label =
		    identity.object_id.empty() ? index_label : index_label + " " + identity.object_id;

		view_node_t group_row;
		group_row.type = "FRMR";
		group_row.size = 0;
		group_row.label = ref_label;
		group_row.values.resize(col_count, non_existent_value);
		group_row.binary_ranges.resize(col_count);
		group_row.cell_conflict_this.resize(col_count, conflict_this_t::unknown);
		group_row.row_conflict_all = conflict_all_t::only_one;

		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= col_refs.size())
				continue;

			for (const auto & ref_group : col_refs[col])
			{
				if (!(ref_group.identity == identity))
					continue;

				group_row.values[col] = std::to_string(ref_group.object_index);
				group_row.binary_ranges[col] = { static_cast<int>(ref_group.start_idx),
					                             static_cast<int>(ref_group.start_idx) + 1 };
				break;
			}
		}

		for (const auto & slot : ref_slots)
		{
			auto child = build_ref_child(col_count, all_subs, col_refs, identity, slot);
			propagate_conflict_upward(group_row, child, col_count);
			group_row.children.push_back(std::move(child));
		}

		group_row.all_identical = check_all_identical(group_row.values);

		bool present_in_any = false;
		for (size_t col = 0; col < col_count; ++col)
		{
			if (group_row.values[col] != non_existent_value)
			{
				present_in_any = true;
				break;
			}
		}

		if (!present_in_any)
			group_row.row_conflict_all = conflict_all_t::only_one;

		for (const auto & child : group_row.children)
		{
			if (child.type == "DELE")
			{
				group_row.is_deleted = true;
				break;
			}
		}

		compute_group_ranges(group_row, col_count);

		return group_row;
	};

	auto build_section_group = [&](const std::string & section_label,
	                               const std::vector<ref_key_t> & identities) -> view_node_t
	{
		view_node_t section;
		section.type = "FRMR";
		section.size = 0;
		section.label = section_label;
		section.values.resize(col_count);
		section.cell_conflict_this.resize(col_count, conflict_this_t::unknown);
		section.row_conflict_all = conflict_all_t::only_one;

		for (const auto & identity : identities)
		{
			auto ref_group = build_ref_group(identity);
			propagate_conflict_upward(section, ref_group, col_count);
			section.children.push_back(std::move(ref_group));
		}

		for (size_t col = 0; col < col_count; ++col)
		{
			bool any_present = false;
			if (col < col_refs.size())
			{
				for (const auto & ref_group : col_refs[col])
				{
					for (const auto & identity : identities)
					{
						if (ref_group.identity == identity)
						{
							any_present = true;
							break;
						}
					}

					if (any_present)
						break;
				}
			}

			char count_buf[32];
			std::snprintf(count_buf, sizeof(count_buf), "%zu references", identities.size());
			section.values[col] = any_present ? std::string(count_buf) : non_existent_value;
		}

		section.all_identical = check_all_identical(section.values);
		compute_group_ranges(section, col_count);
		return section;
	};

	if (!persistent_identities.empty())
		m_rows.push_back(build_section_group("Persistent", persistent_identities));

	if (!temporary_identities.empty())
		m_rows.push_back(build_section_group("Temporary", temporary_identities));
}
