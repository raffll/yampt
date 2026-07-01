#include "view_tree_model.hpp"
#include <decoder/view_tree_format.hpp>
#include <scanner/conflict_compute.hpp>
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
	row.label = make_sub_label(slot.type, m_record_type, first_size);

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

	const auto * schema = find_schema(m_record_type, slot.type, first_size);
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
