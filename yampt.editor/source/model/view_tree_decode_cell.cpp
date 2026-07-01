#include "view_tree_model.hpp"
#include <decoder/view_tree_format.hpp>
#include <scanner/conflict_compute.hpp>
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
    std::vector<view_tree_model_t::sub_slot_t> & header_slots)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		std::unordered_map<std::string, int> type_count;
		for (size_t i = 0; i < col_header_end[col]; ++i)
		{
			const auto & sv = all_subs[col][i];
			int occur = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : header_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occur)
				{
					found = true;
					break;
				}
			}

			if (!found)
				header_slots.push_back({ sv.type, occur });
		}
	}
}

static void build_ref_slots_for_object(
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::vector<cell_ref_group_t>> & col_refs,
    uint32_t object_index,
    std::vector<view_tree_model_t::sub_slot_t> & ref_slots)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (const auto & ref_group : col_refs[col])
		{
			if (ref_group.object_index != object_index)
				continue;

			std::unordered_map<std::string, int> type_count;
			for (size_t i = ref_group.start_idx; i < ref_group.end_idx; ++i)
			{
				const auto & sv = all_subs[col][i];
				int occur = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : ref_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occur)
					{
						found = true;
						break;
					}
				}

				if (!found)
					ref_slots.push_back({ sv.type, occur });
			}
			break;
		}
	}
}

static sub_record_view_t find_ref_sub_record(
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
				return subs[i];

			++occur;
		}
		break;
	}

	return { "", nullptr, 0, 0 };
}

void view_tree_model_t::decode_schema_children_ref(
    sub_record_row_t & parent_row,
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

				field_row_t frow;
				frow.name = fdef.flag_names[bit];
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

		field_row_t frow;
		frow.name = fdef.name;
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
    sub_record_row_t & parent_row,
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
		field_row_t frow;
		char name_buf[16];
		std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
		frow.name = name_buf;
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

		for (const auto & slot : ref_slots)
		{
			sub_record_row_t row;
			row.size = 0;
			row.values.resize(col_count);
			const char * first_data = nullptr;
			size_t first_size = 0;

			for (size_t col = 0; col < col_count; ++col)
			{
				if (col >= all_subs.size())
				{
					row.values[col] = "";
					continue;
				}

				const auto sv = find_ref_sub_record(all_subs[col], col_refs[col], obj_idx, slot.type, slot.occurrence);

				if (!sv.data)
				{
					row.values[col] = "";
					continue;
				}

				if (!first_data)
				{
					first_data = sv.data;
					first_size = sv.size;
					row.size = sv.size;
				}
				row.values[col] = format_value(sv.data, sv.size);
			}

			row.type = slot.type;
			row.label = make_sub_label(slot.type, m_record_type, first_size);
			row.all_identical = check_all_identical(row.values);
			row.row_conflict_all = compute_conflict_all(row.values);
			row.cell_conflict_this = compute_conflict_this(row.values);

			const auto * schema = find_schema(m_record_type, slot.type, first_size);
			if (schema && first_data)
				decode_schema_children_ref(
				    row, schema, first_data, first_size, col_count, all_subs, col_refs, obj_idx, slot);
			else if (
			    first_data && first_size > 0 && !row.values.empty() && !row.values[0].empty() &&
			    row.values[0][0] == '<')
				decode_hex_children_ref(row, first_size, col_count, all_subs, col_refs, obj_idx, slot);

			m_rows.push_back(std::move(row));
		}
	}
}
