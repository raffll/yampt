#include "view_tree_model.hpp"
#include <plugin_scan/view_tree_format.hpp>
#include <plugin_scan/conflict_compute.hpp>
#include <cstring>
#include <cstdio>
#include <algorithm>

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

view_tree_model_t::sub_record_row_t view_tree_model_t::build_slot_row(
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
    const sub_slot_t & slot)
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

		auto it_type = col_indices[col].find(slot.type);
		if (it_type == col_indices[col].end())
		{
			row.values[col] = "";
			continue;
		}

		const auto & indices = it_type->second;
		if (slot.occurrence >= static_cast<int>(indices.size()))
		{
			row.values[col] = "";
			continue;
		}

		size_t idx = indices[slot.occurrence];
		if (idx == SIZE_MAX)
		{
			row.values[col] = "";
			continue;
		}

		const auto & sv = all_subs[col][idx];
		if (!first_data)
		{
			first_data = sv.data;
			first_size = sv.size;
			row.size = sv.size;
		}

		row.values[col] = format_value(sv.data, sv.size);
	}

	row.type = slot.type;
	row.label = make_sub_label(slot.type, record_type_, first_size);

	bool all_same = true;
	for (size_t col = 1; col < col_count; ++col)
	{
		if (row.values[col] != row.values[0])
		{
			all_same = false;
			break;
		}
	}
	row.all_identical = all_same;
	row.row_conflict_all = compute_conflict_all(row.values);
	row.cell_conflict_this = compute_conflict_this(row.values);

	const auto * schema = find_schema(record_type_, slot.type, first_size);
	if (schema && first_data)
		decode_schema_children(row, schema, first_data, first_size, col_count, all_subs, col_indices, slot);
	else if (first_data && first_size > 0 && !row.values.empty() && !row.values[0].empty() && row.values[0][0] == '<')
		decode_hex_children(row, first_size, col_count, all_subs, col_indices, slot);

	return row;
}

void view_tree_model_t::decode_schema_children(
    sub_record_row_t & parent_row,
    const sub_record_schema_t * schema,
    const char *,
    size_t,
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
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

					auto it_type = col_indices[col].find(slot.type);
					if (it_type == col_indices[col].end() ||
					    slot.occurrence >= static_cast<int>(it_type->second.size()))
					{
						frow.values[col] = "";
						continue;
					}

					size_t idx = it_type->second[slot.occurrence];
					if (idx == SIZE_MAX)
					{
						frow.values[col] = "";
						continue;
					}

					frow.values[col] = read_flag_value(all_subs[col][idx], fdef, bit);
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

			auto it_type = col_indices[col].find(slot.type);
			if (it_type == col_indices[col].end() || slot.occurrence >= static_cast<int>(it_type->second.size()))
			{
				frow.values[col] = "";
				continue;
			}

			size_t idx = it_type->second[slot.occurrence];
			if (idx == SIZE_MAX)
			{
				frow.values[col] = "";
				continue;
			}

			const auto & sv = all_subs[col][idx];
			frow.values[col] = decode_field(fdef, sv.data, sv.size);
		}

		frow.all_identical = check_all_identical(frow.values);
		frow.row_conflict_all = compute_conflict_all(frow.values);
		frow.cell_conflict_this = compute_conflict_this(frow.values);
		parent_row.children.push_back(std::move(frow));
	}
}

void view_tree_model_t::decode_hex_children(
    sub_record_row_t & parent_row,
    size_t first_size,
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
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

			auto it_type = col_indices[col].find(slot.type);
			if (it_type == col_indices[col].end() || slot.occurrence >= static_cast<int>(it_type->second.size()))
			{
				frow.values[col] = "";
				continue;
			}

			size_t idx = it_type->second[slot.occurrence];
			if (idx == SIZE_MAX)
			{
				frow.values[col] = "";
				continue;
			}

			const auto & sv = all_subs[col][idx];
			frow.values[col] = format_hex_chunk(sv.data, sv.size, offset);
		}

		frow.all_identical = check_all_identical(frow.values);
		frow.row_conflict_all = compute_conflict_all(frow.values);
		frow.cell_conflict_this = compute_conflict_this(frow.values);
		parent_row.children.push_back(std::move(frow));
	}
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
		rows_.push_back(std::move(row));
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
			row.label = make_sub_label(slot.type, record_type_, first_size);
			row.all_identical = check_all_identical(row.values);
			row.row_conflict_all = compute_conflict_all(row.values);
			row.cell_conflict_this = compute_conflict_this(row.values);

			const auto * schema = find_schema(record_type_, slot.type, first_size);
			if (schema && first_data)
				decode_schema_children_ref(
				    row, schema, first_data, first_size, col_count, all_subs, col_refs, obj_idx, slot);
			else if (
			    first_data && first_size > 0 && !row.values.empty() && !row.values[0].empty() &&
			    row.values[0][0] == '<')
				decode_hex_children_ref(row, first_size, col_count, all_subs, col_refs, obj_idx, slot);

			rows_.push_back(std::move(row));
		}
	}
}

static void build_unified_slots_from_alignment(
    const slot_result_t & slot_result,
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots)
{
	for (const auto & aligned_slot : slot_result.aligned)
		unified_slots.push_back({ aligned_slot.key.type, aligned_slot.key.occurrence });
}

static void build_col_indices_from_alignment(
    const slot_result_t & slot_result,
    size_t col_count,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices)
{
	for (const auto & aligned_slot : slot_result.aligned)
	{
		const auto ver_size = aligned_slot.indices.size();
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col < ver_size)
				col_indices[col][aligned_slot.key.type].push_back(aligned_slot.indices[col]);
			else
				col_indices[col][aligned_slot.key.type].push_back(SIZE_MAX);
		}
	}
}

template<typename Predicate>
static void build_non_excluded_slots(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    Predicate is_excluded,
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots)
{
	std::unordered_map<std::string, int> type_count;
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (const auto & sv : all_subs[col])
		{
			if (is_excluded(sv))
				continue;

			int occur = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : unified_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occur)
				{
					found = true;
					break;
				}
			}

			if (!found)
				unified_slots.push_back({ sv.type, occur });
		}
		type_count.clear();
	}
}

static void append_pair_slots(
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots,
    const std::vector<std::string> & entry_keys,
    const std::string & type_first,
    const std::string & type_second)
{
	for (size_t i = 0; i < entry_keys.size(); ++i)
	{
		int occ_first = -1;
		int occ_second = -1;
		for (const auto & slot : unified_slots)
		{
			if (slot.type == type_first)
				occ_first = std::max(occ_first, slot.occurrence);

			if (slot.type == type_second)
				occ_second = std::max(occ_second, slot.occurrence);
		}
		unified_slots.push_back({ type_first, occ_first + 1 });
		unified_slots.push_back({ type_second, occ_second + 1 });
	}
}

static void append_single_slots(
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots,
    const std::vector<std::string> & entry_keys,
    const std::string & slot_type)
{
	for (size_t i = 0; i < entry_keys.size(); ++i)
	{
		int max_occ = -1;
		for (const auto & slot : unified_slots)
		{
			if (slot.type == slot_type)
				max_occ = std::max(max_occ, slot.occurrence);
		}
		unified_slots.push_back({ slot_type, max_occ + 1 });
	}
}

template<typename Predicate>
static void build_non_excluded_indices(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    Predicate is_excluded,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (size_t i = 0; i < all_subs[col].size(); ++i)
		{
			if (is_excluded(all_subs[col][i]))
				continue;

			col_indices[col][all_subs[col][i].type].push_back(i);
		}
	}
}

struct paired_entry_t
{
	std::string entry_id;
	size_t first_idx;
	size_t second_idx;
};

struct single_entry_t
{
	std::string entry_id;
	size_t sub_idx;
};

static void match_paired_entries(
    const std::vector<std::vector<paired_entry_t>> & col_entries,
    const std::vector<std::string> & all_ids,
    const std::string & type_first,
    const std::string & type_second,
    size_t col_count,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= col_entries.size())
			continue;

		for (const auto & target_id : all_ids)
		{
			bool matched = false;
			for (const auto & entry : col_entries[col])
			{
				if (entry.entry_id != target_id)
					continue;

				col_type_indices[col][type_first].push_back(entry.first_idx);
				col_type_indices[col][type_second].push_back(entry.second_idx);
				matched = true;
				break;
			}

			if (!matched)
			{
				col_type_indices[col][type_first].push_back(SIZE_MAX);
				col_type_indices[col][type_second].push_back(SIZE_MAX);
			}
		}
	}
}

static void match_single_entries(
    const std::vector<std::vector<single_entry_t>> & col_entries,
    const std::vector<std::string> & all_ids,
    const std::string & slot_type,
    size_t col_count,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= col_entries.size())
			continue;

		for (const auto & target_id : all_ids)
		{
			bool matched = false;
			for (const auto & entry : col_entries[col])
			{
				if (entry.entry_id != target_id)
					continue;

				col_type_indices[col][slot_type].push_back(entry.sub_idx);
				matched = true;
				break;
			}

			if (!matched)
				col_type_indices[col][slot_type].push_back(SIZE_MAX);
		}
	}
}

void view_tree_model_t::set_record_leveled(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
		build_col_indices_from_alignment(*entry.slot_result, col_count, col_type_indices);
	}
	else
	{
		collect_leveled_entries(context, build_ctx);
	}

	emit_slot_rows(context, build_ctx);
}

void view_tree_model_t::set_record_faction(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
		build_col_indices_from_alignment(*entry.slot_result, col_count, col_type_indices);
	}
	else
	{
		collect_faction_entries(context, build_ctx);
	}

	emit_slot_rows(context, build_ctx);
}

void view_tree_model_t::set_record_container(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
		build_col_indices_from_alignment(*entry.slot_result, col_count, col_type_indices);
	}
	else
	{
		collect_container_entries(context, build_ctx);
	}

	emit_slot_rows(context, build_ctx);
}

void view_tree_model_t::set_record_generic(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<sub_slot_t> unified_slots;

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
	}
	else
	{
		for (const auto & subs : all_subs)
		{
			std::unordered_map<std::string, int> type_count;
			for (const auto & sv : subs)
			{
				int occur = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occur)
					{
						found = true;
						break;
					}
				}

				if (!found)
					unified_slots.push_back({ sv.type, occur });
			}
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (size_t i = 0; i < all_subs[col].size(); ++i)
			col_type_indices[col][all_subs[col][i].type].push_back(i);
	}

	for (const auto & slot : unified_slots)
		rows_.push_back(build_slot_row(col_count, all_subs, col_type_indices, slot));
}

void view_tree_model_t::collect_leveled_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	static constexpr size_t leveled_intv_size = 2;
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<std::vector<paired_entry_t>> col_entries(col_count);
	std::vector<std::string> all_item_ids;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i + 1 < subs.size(); ++i)
		{
			if (subs[i].type != "INTV" || subs[i].size != leveled_intv_size || subs[i + 1].type != "INAM")
				continue;

			std::string item_id(subs[i + 1].data, subs[i + 1].size);
			if (!item_id.empty() && item_id.back() == '\0')
				item_id.pop_back();

			col_entries[col].push_back({ item_id, i, i + 1 });

			if (std::find(all_item_ids.begin(), all_item_ids.end(), item_id) == all_item_ids.end())
				all_item_ids.push_back(item_id);
		}
	}

	auto is_excluded = [](const sub_record_view_t & sv_rec)
	{ return (sv_rec.type == "INTV" && sv_rec.size == 2) || sv_rec.type == "INAM"; };

	build_non_excluded_slots(all_subs, col_count, is_excluded, build_ctx.unified_slots);
	append_pair_slots(build_ctx.unified_slots, all_item_ids, "INTV", "INAM");
	build_non_excluded_indices(all_subs, col_count, is_excluded, build_ctx.col_type_indices);
	match_paired_entries(col_entries, all_item_ids, "INTV", "INAM", col_count, build_ctx.col_type_indices);
}

void view_tree_model_t::collect_faction_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	static constexpr size_t faction_intv_size = 4;
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<std::vector<paired_entry_t>> col_entries(col_count);
	std::vector<std::string> all_faction_names;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i + 1 < subs.size(); ++i)
		{
			if (subs[i].type != "INTV" || subs[i].size != faction_intv_size || subs[i + 1].type != "ANAM")
				continue;

			std::string faction_name(subs[i + 1].data, subs[i + 1].size);
			if (!faction_name.empty() && faction_name.back() == '\0')
				faction_name.pop_back();

			col_entries[col].push_back({ faction_name, i, i + 1 });

			if (std::find(all_faction_names.begin(), all_faction_names.end(), faction_name) == all_faction_names.end())
				all_faction_names.push_back(faction_name);
		}
	}

	auto is_excluded = [](const sub_record_view_t & sv_rec)
	{ return (sv_rec.type == "INTV" && sv_rec.size == 4) || sv_rec.type == "ANAM"; };

	build_non_excluded_slots(all_subs, col_count, is_excluded, build_ctx.unified_slots);
	append_pair_slots(build_ctx.unified_slots, all_faction_names, "INTV", "ANAM");
	build_non_excluded_indices(all_subs, col_count, is_excluded, build_ctx.col_type_indices);
	match_paired_entries(col_entries, all_faction_names, "INTV", "ANAM", col_count, build_ctx.col_type_indices);
}

void view_tree_model_t::collect_container_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	static constexpr size_t npco_record_size = 36;
	static constexpr size_t npco_name_offset = 4;
	static constexpr size_t npco_name_length = 32;
	static constexpr size_t npcs_record_size = 32;

	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<std::vector<single_entry_t>> col_items(col_count);
	std::vector<std::vector<single_entry_t>> col_spells(col_count);
	std::vector<std::string> all_item_ids;
	std::vector<std::string> all_spell_ids;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i < subs.size(); ++i)
		{
			if (subs[i].type == "NPCO" && subs[i].size == npco_record_size)
			{
				std::string item_id(subs[i].data + npco_name_offset, npco_name_length);
				while (!item_id.empty() && item_id.back() == '\0')
					item_id.pop_back();

				col_items[col].push_back({ item_id, i });
				if (std::find(all_item_ids.begin(), all_item_ids.end(), item_id) == all_item_ids.end())
					all_item_ids.push_back(item_id);
			}
			else if (subs[i].type == "NPCS" && subs[i].size == npcs_record_size)
			{
				std::string spell_id(subs[i].data, npcs_record_size);
				while (!spell_id.empty() && spell_id.back() == '\0')
					spell_id.pop_back();

				col_spells[col].push_back({ spell_id, i });
				if (std::find(all_spell_ids.begin(), all_spell_ids.end(), spell_id) == all_spell_ids.end())
					all_spell_ids.push_back(spell_id);
			}
		}
	}

	auto is_excluded = [](const sub_record_view_t & sv_rec)
	{ return (sv_rec.type == "NPCO" && sv_rec.size == 36) || (sv_rec.type == "NPCS" && sv_rec.size == 32); };
	build_non_excluded_slots(all_subs, col_count, is_excluded, build_ctx.unified_slots);
	append_single_slots(build_ctx.unified_slots, all_item_ids, "NPCO");
	append_single_slots(build_ctx.unified_slots, all_spell_ids, "NPCS");
	build_non_excluded_indices(all_subs, col_count, is_excluded, build_ctx.col_type_indices);
	match_single_entries(col_items, all_item_ids, "NPCO", col_count, build_ctx.col_type_indices);
	match_single_entries(col_spells, all_spell_ids, "NPCS", col_count, build_ctx.col_type_indices);
}

void view_tree_model_t::emit_slot_rows(record_context_t & context, slot_build_context_t & build_ctx)
{
	for (const auto & slot : build_ctx.unified_slots)
		rows_.push_back(build_slot_row(context.col_count, context.all_sub_records, build_ctx.col_type_indices, slot));
}
