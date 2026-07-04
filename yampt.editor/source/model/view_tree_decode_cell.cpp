#include "view_tree_model.hpp"
#include <decoder/view_tree_format.hpp>
#include <scanner/record_conflict.hpp>
#include <cstring>

static constexpr bool show_binary_positions = true;

static bool check_all_identical(const std::vector<std::string> & values)
{
	for (size_t col = 1; col < values.size(); ++col)
	{
		if (values[col] != values[0])
			return false;
	}
	return true;
}

static std::string read_flag_value(const sub_record_view_t & sv, const field_def_t & fdef, int bit_index)
{
	if (fdef.offset >= sv.size)
		return "";

	uint32_t value = 0;
	const size_t byte_count = (fdef.type == field_type_t::flags_u8)    ? 1
	                          : (fdef.type == field_type_t::flags_u16) ? 2
	                                                                   : 4;
	std::memcpy(&value, sv.data + fdef.offset, std::min(byte_count, sv.size - fdef.offset));
	return (value & (1u << bit_index)) ? "1" : "0";
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
	static constexpr size_t frmr_data_size = 4;
	uint32_t obj_idx = 0;
	if (sub_rec.size >= frmr_data_size)
		std::memcpy(&obj_idx, sub_rec.data, frmr_data_size);

	return obj_idx;
}

static void collect_cell_ref_groups(
    const std::vector<sub_record_view_t> & subs,
    std::vector<cell_ref_group_t> & out_refs,
    size_t & out_header_end)
{
	for (size_t i = 0; i < subs.size(); ++i)
	{
		if (subs[i].type != "FRMR")
			continue;

		if (out_refs.empty())
			out_header_end = i;

		const auto obj_idx = read_object_index(subs[i]);

		size_t group_end = subs.size();
		for (size_t j = i + 1; j < subs.size(); ++j)
		{
			if (subs[j].type == "FRMR")
			{
				group_end = j;
				break;
			}
		}

		out_refs.push_back({ obj_idx, i, group_end });
	}

	if (out_refs.empty())
		out_header_end = subs.size();
}

static void collect_unique_object_indices(
    const std::vector<std::vector<cell_ref_group_t>> & col_refs,
    std::vector<uint32_t> & all_indices)
{
	for (const auto & refs : col_refs)
	{
		for (const auto & ref_group : refs)
		{
			bool found = false;
			for (const auto & existing : all_indices)
			{
				if (existing == ref_group.object_index)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_indices.push_back(ref_group.object_index);
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
    const std::vector<std::vector<cell_ref_group_t>> & col_refs,
    uint32_t object_index,
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
			if (ref_group.object_index != object_index)
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
    const std::vector<cell_ref_group_t> & refs,
    uint32_t object_index,
    const std::string & slot_type,
    int slot_occurrence)
{
	for (const auto & ref_group : refs)
	{
		if (ref_group.object_index != object_index)
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
    const std::vector<std::vector<cell_ref_group_t>> & col_refs,
    uint32_t object_index,
    const sub_slot_t & slot)
{
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
				frow.values.resize(col_count);

				for (size_t col = 0; col < col_count; ++col)
				{
					if (col >= all_subs.size())
					{
						frow.values[col] = "";
						continue;
					}

					frow.values[col] = "";
					for (const auto & ref_group : col_refs[col])
					{
						if (ref_group.object_index != object_index)
							continue;

						int occur = 0;
						for (size_t i = ref_group.start_idx; i < ref_group.end_idx; ++i)
						{
							const auto & sv = all_subs[col][i];
							if (sv.type != slot.type)
								continue;

							if (occur == slot.occurrence)
							{
								frow.values[col] = read_flag_value(sv, fdef, bit);
								break;
							}
							++occur;
						}
						break;
					}
				}

				frow.all_identical = check_all_identical(frow.values);
				frow.row_conflict_all = compute_conflict_all(frow.values);
				frow.cell_conflict_this = compute_conflict_this(frow.values);
				parent_row.children.push_back(std::move(frow));
			}
			continue;
		}

		view_node_t frow;
		frow.label = fdef.name;
		frow.values.resize(col_count);

		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_subs.size())
			{
				frow.values[col] = "";
				continue;
			}

			frow.values[col] = "";
			for (const auto & ref_group : col_refs[col])
			{
				if (ref_group.object_index != object_index)
					continue;

				int occur = 0;
				for (size_t i = ref_group.start_idx; i < ref_group.end_idx; ++i)
				{
					const auto & sv = all_subs[col][i];
					if (sv.type != slot.type)
						continue;

					if (occur == slot.occurrence)
					{
						frow.values[col] = decode_field(fdef, sv.data, sv.size);
						break;
					}
					++occur;
				}
				break;
			}
		}

		frow.all_identical = check_all_identical(frow.values);
		frow.row_conflict_all = compute_conflict_all(frow.values);
		frow.cell_conflict_this = compute_conflict_this(frow.values);
		parent_row.children.push_back(std::move(frow));
	}
}

void view_tree_model_t::decode_hex_children_ref(
    view_node_t & parent_row,
    size_t first_size,
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::vector<cell_ref_group_t>> & col_refs,
    uint32_t object_index,
    const sub_slot_t & slot)
{
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
			if (col >= all_subs.size())
			{
				frow.values[col] = "";
				continue;
			}

			frow.values[col] = "";
			for (const auto & ref_group : col_refs[col])
			{
				if (ref_group.object_index != object_index)
					continue;

				int occur = 0;
				for (size_t i = ref_group.start_idx; i < ref_group.end_idx; ++i)
				{
					const auto & sv = all_subs[col][i];
					if (sv.type != slot.type)
						continue;

					if (occur == slot.occurrence)
					{
						frow.values[col] = format_hex_chunk(sv.data, sv.size, offset);
						break;
					}
					++occur;
				}
				break;
			}
		}

		frow.all_identical = check_all_identical(frow.values);
		frow.row_conflict_all = compute_conflict_all(frow.values);
		frow.cell_conflict_this = compute_conflict_this(frow.values);
		parent_row.children.push_back(std::move(frow));
	}
}

void view_tree_model_t::set_record_cell(record_context_t & context)
{
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<std::vector<cell_ref_group_t>> col_refs(col_count);
	std::vector<size_t> col_header_end(col_count, 0);
	std::vector<uint32_t> all_object_indices;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		collect_cell_ref_groups(all_subs[col], col_refs[col], col_header_end[col]);
	}

	collect_unique_object_indices(col_refs, all_object_indices);

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
		auto row = build_slot_row(col_count, all_subs, col_header_indices, slot);
		m_rows.push_back(std::move(row));
	}

	for (const auto & obj_idx : all_object_indices)
	{
		std::vector<sub_slot_t> ref_slots;
		build_ref_slots_for_object(col_count, all_subs, col_refs, obj_idx, ref_slots);

		std::string ref_label;
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_subs.size())
				continue;

			for (const auto & ref_group : col_refs[col])
			{
				if (ref_group.object_index != obj_idx)
					continue;

				for (size_t i = ref_group.start_idx; i < ref_group.end_idx; ++i)
				{
					if (all_subs[col][i].type == "NAME")
					{
						std::string name_text(all_subs[col][i].data, all_subs[col][i].size);
						if (!name_text.empty() && name_text.back() == '\0')
							name_text.pop_back();

						ref_label = name_text + " #" + std::to_string(obj_idx);
						break;
					}
				}
				break;
			}

			if (!ref_label.empty())
				break;
		}

		if (ref_label.empty())
			ref_label = "#" + std::to_string(obj_idx);

		view_node_t group_row;
		group_row.type = "FRMR";
		group_row.size = 0;

		group_row.label = ref_label;
		group_row.values.resize(col_count);
		group_row.cell_conflict_this.resize(col_count, conflict_this_t::unknown);
		group_row.row_conflict_all = conflict_all_t::only_one;

		for (const auto & slot : ref_slots)
		{
			const char * first_data = nullptr;
			size_t first_size = 0;

			for (size_t col = 0; col < col_count; ++col)
			{
				if (col >= all_subs.size())
					continue;

				const auto result = find_ref_sub_record(all_subs[col], col_refs[col], obj_idx, slot.type, slot.occurrence);
				if (result.view.data && !first_data)
				{
					first_data = result.view.data;
					first_size = result.view.size;
				}
			}

			const auto * schema = first_data ? find_schema(m_record_type, slot.type, first_size) : nullptr;
			if (!schema && first_data)
				schema = find_schema("*", slot.type, first_size);

			if (schema && schema->field_count > 1)
			{
				for (size_t field_idx = 0; field_idx < schema->field_count; ++field_idx)
				{
					const auto & fdef = schema->fields[field_idx];
					view_node_t field_child;
					field_child.label = std::string(slot.type) + " - " + fdef.name;
					field_child.type = slot.type;
					field_child.size = first_size;
					field_child.values.resize(col_count);
					field_child.binary_ranges.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_subs.size())
						{
							field_child.values[col] = "";
							continue;
						}

						const auto result = find_ref_sub_record(all_subs[col], col_refs[col], obj_idx, slot.type, slot.occurrence);
						if (!result.view.data)
							field_child.values[col] = "";
						else
						{
							field_child.values[col] = decode_field(fdef, result.view.data, result.view.size);
							field_child.binary_ranges[col] = { result.binary_index, result.binary_index + 1 };
							if (show_binary_positions)
								field_child.values[col] = "[" + std::to_string(result.binary_index) + "] " + field_child.values[col];
						}
					}

					field_child.all_identical = check_all_identical(field_child.values);
					field_child.row_conflict_all = compute_conflict_all(field_child.values);
					field_child.cell_conflict_this = compute_conflict_this(field_child.values);

					if (field_child.row_conflict_all > group_row.row_conflict_all)
						group_row.row_conflict_all = field_child.row_conflict_all;

					for (size_t col = 0; col < col_count && col < field_child.cell_conflict_this.size(); ++col)
					{
						if (field_child.cell_conflict_this[col] > group_row.cell_conflict_this[col])
							group_row.cell_conflict_this[col] = field_child.cell_conflict_this[col];
					}

					group_row.children.push_back(std::move(field_child));
				}
			}
			else
			{
				view_node_t child_field;
				child_field.type = slot.type;
				child_field.size = first_size;
				child_field.values.resize(col_count);
				child_field.binary_ranges.resize(col_count);

				for (size_t col = 0; col < col_count; ++col)
				{
					if (col >= all_subs.size())
					{
						child_field.values[col] = "";
						continue;
					}

					const auto result = find_ref_sub_record(all_subs[col], col_refs[col], obj_idx, slot.type, slot.occurrence);
					if (!result.view.data)
					{
						child_field.values[col] = "";
						continue;
					}

					child_field.binary_ranges[col] = { result.binary_index, result.binary_index + 1 };
					if (schema && schema->field_count == 1)
						child_field.values[col] = decode_field(schema->fields[0], result.view.data, result.view.size);
					else
						child_field.values[col] = format_value(result.view.data, result.view.size, m_display_codepage);

					if (show_binary_positions)
						child_field.values[col] = "[" + std::to_string(result.binary_index) + "] " + child_field.values[col];
				}

				child_field.label = slot.type + " - " + make_sub_label(slot.type, m_record_type, first_size);
				child_field.all_identical = check_all_identical(child_field.values);
				child_field.row_conflict_all = compute_conflict_all(child_field.values);
				child_field.cell_conflict_this = compute_conflict_this(child_field.values);

				if (child_field.row_conflict_all > group_row.row_conflict_all)
					group_row.row_conflict_all = child_field.row_conflict_all;

				for (size_t col = 0; col < col_count && col < child_field.cell_conflict_this.size(); ++col)
				{
					if (child_field.cell_conflict_this[col] > group_row.cell_conflict_this[col])
						group_row.cell_conflict_this[col] = child_field.cell_conflict_this[col];
				}

				group_row.children.push_back(std::move(child_field));
			}
		}

		bool present_in_any = false;
		for (size_t col = 0; col < col_count; ++col)
		{
			bool present = false;
			if (col < col_refs.size())
			{
				for (const auto & ref_group : col_refs[col])
				{
					if (ref_group.object_index == obj_idx)
					{
						present = true;
						break;
					}
				}
			}

			if (present)
			{
				group_row.values[col] = ref_label;
				present_in_any = true;
			}
			else
			{
				group_row.values[col] = "";
			}
		}

		group_row.all_identical = check_all_identical(group_row.values);
		if (!present_in_any)
			group_row.row_conflict_all = conflict_all_t::only_one;

		compute_group_ranges(group_row, col_count);

		group_row.label += " [";
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col > 0)
				group_row.label += " ";

			if (col < group_row.binary_ranges.size() && group_row.binary_ranges[col].start >= 0)
				group_row.label += std::to_string(group_row.binary_ranges[col].start) + ".." + std::to_string(group_row.binary_ranges[col].end_pos);
			else
				group_row.label += "-";
		}
		group_row.label += "]";

		m_rows.push_back(std::move(group_row));
	}
}
