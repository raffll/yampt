#include "view_tree_model.hpp"
#include <decoder/view_tree_format.hpp>
#include <scanner/record_conflict.hpp>
#include <cstdio>
#include <cstring>

static std::vector<std::string> exclude_column(const std::vector<std::string> & values, int excluded_col)
{
	if (excluded_col < 0 || excluded_col >= static_cast<int>(values.size()))
		return values;

	if (!values[excluded_col].empty())
		return values;

	std::vector<std::string> result;
	result.reserve(values.size() - 1);
	for (size_t i = 0; i < values.size(); ++i)
	{
		if (static_cast<int>(i) != excluded_col)
			result.push_back(values[i]);
	}
	return result;
}

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
	return (value & (1u << bit_index)) ? "Yes" : "No";
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

view_tree_model_t::view_node_t view_tree_model_t::build_slot_row(
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
    const sub_slot_t & slot)
{
	view_node_t row;
	row.size = 0;
	row.values.resize(col_count);
	row.binary_indices.resize(col_count, -1);
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

		row.binary_indices[col] = static_cast<int>(idx);
		const auto & sv = all_subs[col][idx];
		if (!first_data)
		{
			first_data = sv.data;
			first_size = sv.size;
			row.size = sv.size;
		}

		row.values[col] = format_value(sv.data, sv.size, m_display_codepage);
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
	const auto & filtered = exclude_column(row.values, m_merge_col_index);
	row.row_conflict_all = compute_conflict_all(filtered);
	row.cell_conflict_this = compute_conflict_this(filtered);
	if (m_merge_col_index >= 0 && filtered.size() < row.values.size())
		row.cell_conflict_this.insert(row.cell_conflict_this.begin() + m_merge_col_index, conflict_this_t::unknown);

	const auto * schema = find_schema(m_record_type, slot.type, first_size);
	if (schema && first_data)
		decode_schema_children(row, schema, first_data, first_size, col_count, all_subs, col_indices, slot);
	else if (first_data && first_size > 0 && !row.values.empty() && !row.values[0].empty() && row.values[0][0] == '<')
		decode_hex_children(row, first_size, col_count, all_subs, col_indices, slot);

	if (!row.children.empty())
	{
		row.row_conflict_all = conflict_all_t::unknown;
		for (const auto & child : row.children)
		{
			if (child.row_conflict_all > row.row_conflict_all)
				row.row_conflict_all = child.row_conflict_all;
		}

		for (size_t col = 0; col < row.cell_conflict_this.size(); ++col)
		{
			conflict_this_t worst = conflict_this_t::unknown;
			for (const auto & child : row.children)
			{
				if (col < child.cell_conflict_this.size() && child.cell_conflict_this[col] > worst)
					worst = child.cell_conflict_this[col];
			}
			row.cell_conflict_this[col] = worst;
		}
	}

	return row;
}

void view_tree_model_t::decode_schema_children(
    view_node_t & parent_row,
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
				const auto & flag_filtered = exclude_column(frow.values, m_merge_col_index);
				frow.row_conflict_all = compute_conflict_all(flag_filtered);
				frow.cell_conflict_this = compute_conflict_this(flag_filtered);
				if (m_merge_col_index >= 0 && flag_filtered.size() < frow.values.size())
					frow.cell_conflict_this.insert(
					    frow.cell_conflict_this.begin() + m_merge_col_index, conflict_this_t::unknown);
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
    view_node_t & parent_row,
    size_t first_size,
    size_t col_count,
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
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
