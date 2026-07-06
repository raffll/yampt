#include "plugin_index.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../decoder/sub_record_schema.hpp"
#include "../utility/app_logger.hpp"
#include "../utility/string_utils.hpp"
#include <algorithm>
#include <set>

static constexpr size_t cell_data_min_size = 12;
static constexpr size_t cell_flags_offset = 0;
static constexpr size_t cell_grid_x_offset = 4;
static constexpr size_t cell_grid_y_offset = 8;
static constexpr size_t grid_coord_size = 4;
static constexpr uint32_t interior_flag_bit = 0x01;
static constexpr size_t indx_min_size = 4;
static constexpr size_t schd_name_length = 32;
static constexpr size_t land_intv_min_size = 8;

static std::string derive_cell_id(sub_record_iter_t & iter, size_t fallback_index)
{
	std::string name_text;
	bool has_name = false;
	int32_t grid_x = 0;
	int32_t grid_y = 0;
	bool is_interior = false;
	bool has_data = false;

	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type == "NAME")
		{
			name_text = std::string(sub.data, sub.size);
			name_text = string_utils::erase_null_chars(name_text);
			has_name = true;
			continue;
		}

		if (sub.type == "DATA" && sub.size >= cell_data_min_size)
		{
			uint32_t flags = static_cast<uint32_t>(domain_types::convert_string_byte_array_to_uint(
			    std::string(sub.data + cell_flags_offset, grid_coord_size)));
			is_interior = (flags & interior_flag_bit) != 0;
			grid_x = static_cast<int32_t>(domain_types::convert_string_byte_array_to_uint(
			    std::string(sub.data + cell_grid_x_offset, grid_coord_size)));
			grid_y = static_cast<int32_t>(domain_types::convert_string_byte_array_to_uint(
			    std::string(sub.data + cell_grid_y_offset, grid_coord_size)));
			has_data = true;
			break;
		}
	}

	if (is_interior && has_name)
		return name_text;

	if (has_data && !is_interior)
		return "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";

	if (has_name)
		return name_text;

	return std::to_string(fallback_index);
}

static std::string derive_index_based_id(sub_record_iter_t & iter, size_t fallback_index)
{
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != "INDX")
			continue;

		if (sub.size < indx_min_size)
			break;

		int32_t index_val = static_cast<int32_t>(
		    domain_types::convert_string_byte_array_to_uint(std::string(sub.data, grid_coord_size)));
		return std::to_string(index_val);
	}

	return std::to_string(fallback_index);
}

static std::string derive_script_id(sub_record_iter_t & iter, size_t fallback_index)
{
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != "SCHD")
			continue;

		const size_t name_len = std::min(sub.size, schd_name_length);
		std::string script_name(sub.data, name_len);
		script_name = string_utils::erase_null_chars(script_name);
		return script_name;
	}

	return std::to_string(fallback_index);
}

static std::string derive_sub_text_id(sub_record_iter_t & iter, const std::string & sub_type, size_t fallback_index)
{
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != sub_type)
			continue;

		std::string text(sub.data, sub.size);
		text = string_utils::erase_null_chars(text);
		return text;
	}

	return std::to_string(fallback_index);
}

static std::string derive_land_id(sub_record_iter_t & iter, size_t fallback_index)
{
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != "INTV")
			continue;

		if (sub.size < land_intv_min_size)
			break;

		int32_t grid_x = static_cast<int32_t>(
		    domain_types::convert_string_byte_array_to_uint(std::string(sub.data, grid_coord_size)));
		int32_t grid_y = static_cast<int32_t>(
		    domain_types::convert_string_byte_array_to_uint(std::string(sub.data + grid_coord_size, grid_coord_size)));
		return "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";
	}

	return std::to_string(fallback_index);
}

static std::string derive_pgrd_id(sub_record_iter_t & iter, size_t fallback_index)
{
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != "DATA")
			continue;

		if (sub.size < land_intv_min_size)
			break;

		int32_t grid_x = static_cast<int32_t>(
		    domain_types::convert_string_byte_array_to_uint(std::string(sub.data, grid_coord_size)));
		int32_t grid_y = static_cast<int32_t>(
		    domain_types::convert_string_byte_array_to_uint(std::string(sub.data + grid_coord_size, grid_coord_size)));
		return "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";
	}

	return std::to_string(fallback_index);
}

plugin_index_t::plugin_index_t(esm_reader_t & esm)
{
	if (!esm.is_loaded())
		return;

	const auto & records = esm.get_records();
	std::string current_dial;

	for (size_t i = 0; i < records.size(); ++i)
	{
		esm.select_record(i);
		const auto & rec_type = esm.get_record().id;

		indexed_record_t entry;
		entry.rec_type = rec_type;
		entry.record_index = i;
		entry.record_id = derive_id(esm, i);
		entry.display_name = derive_display_name(esm, i);

		if (rec_type == "DIAL")
			current_dial = entry.record_id;

		if (rec_type == "INFO")
		{
			entry.dial_name = current_dial;
			entry.record_id = current_dial + "|" + entry.record_id;
		}

		sub_record_iter_t iter(esm.get_record().content);
		sub_record_view_t sub;
		while (iter.next(sub))
		{
			if (sub.type == "DELE")
			{
				entry.has_dele = true;
				break;
			}
		}

		std::string key = rec_type + std::string(1, '\0') + entry.record_id;
		if (m_lookup.find(key) == m_lookup.end())
			m_lookup[key] = m_entries.size();

		m_entries.push_back(std::move(entry));
	}
}

const std::vector<indexed_record_t> & plugin_index_t::entries() const
{
	return m_entries;
}

const indexed_record_t * plugin_index_t::find(const std::string & type, const std::string & id) const
{
	std::string key = type + std::string(1, '\0') + id;
	auto it = m_lookup.find(key);
	if (it == m_lookup.end())
		return nullptr;

	return &m_entries[it->second];
}

std::vector<std::string> plugin_index_t::types() const
{
	std::set<std::string> unique;
	for (const auto & entry : m_entries)
		unique.insert(entry.rec_type);

	return std::vector<std::string>(unique.begin(), unique.end());
}

size_t plugin_index_t::count_by_type(const std::string & type) const
{
	size_t count = 0;
	for (const auto & entry : m_entries)
	{
		if (entry.rec_type == type)
			++count;
	}
	return count;
}

std::string plugin_index_t::derive_id(esm_reader_t & esm, size_t i)
{
	const auto & rec_type = esm.get_record().id;
	const auto & content = esm.get_record().content;

	if (rec_type == "TES3")
		return "";

	sub_record_iter_t iter(content);

	if (rec_type == "CELL")
		return derive_cell_id(iter, i);

	if (rec_type == "SKIL" || rec_type == "MGEF")
		return derive_index_based_id(iter, i);

	if (rec_type == "SCPT")
		return derive_script_id(iter, i);

	if (rec_type == "DIAL")
		return derive_sub_text_id(iter, "NAME", i);

	if (rec_type == "INFO")
		return derive_sub_text_id(iter, "INAM", i);

	if (rec_type == "LAND")
		return derive_land_id(iter, i);

	if (rec_type == "PGRD")
		return derive_pgrd_id(iter, i);

	return derive_sub_text_id(iter, "NAME", i);
}

std::string plugin_index_t::derive_display_name(esm_reader_t & esm, size_t i)
{
	const auto & content = esm.get_record().content;
	const auto & rec_type = esm.get_record().id;

	if (rec_type == "MGEF" || rec_type == "SKIL")
	{
		sub_record_iter_t iter(content);
		sub_record_view_t sub;
		while (iter.next(sub))
		{
			if (sub.type != "INDX")
				continue;

			if (sub.size < indx_min_size)
				break;

			int32_t index_val = static_cast<int32_t>(
			    domain_types::convert_string_byte_array_to_uint(std::string(sub.data, grid_coord_size)));

			if (rec_type == "MGEF")
			{
				const char * name = effect_name_by_index(index_val);
				if (name)
					return name;
			}
			else
			{
				const char * name = skill_name_by_index(index_val);
				if (name)
					return name;
			}

			break;
		}

		return "";
	}

	sub_record_iter_t iter(content);
	sub_record_view_t sub;

	if (rec_type == "INFO")
	{
		while (iter.next(sub))
		{
			if (sub.type != "ONAM")
				continue;

			std::string text(sub.data, sub.size);
			text = string_utils::erase_null_chars(text);
			return text;
		}
		return "";
	}

	while (iter.next(sub))
	{
		if (sub.type != "FNAM")
			continue;

		std::string text(sub.data, sub.size);
		text = string_utils::erase_null_chars(text);
		return text;
	}

	return "";
}
