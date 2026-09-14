#include "plugin_index.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../decoder/sub_record_schema.hpp"
#include "../decoder/view_tree_format.hpp"
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

enum class id_strategy_t
{
	empty,
	cell,
	index_based,
	script,
	sub_text,
	land,
	pgrd
};

struct id_rule_t
{
	const char * rec_type;
	id_strategy_t strategy;
	const char * sub_key;
};

static constexpr id_rule_t id_rule_table[] = {
	{ "TES3", id_strategy_t::empty, nullptr },
	{ "CELL", id_strategy_t::cell, nullptr },
	{ "SKIL", id_strategy_t::index_based, nullptr },
	{ "MGEF", id_strategy_t::index_based, nullptr },
	{ "SCPT", id_strategy_t::script, nullptr },
	{ "DIAL", id_strategy_t::sub_text, "NAME" },
	{ "INFO", id_strategy_t::sub_text, "INAM" },
	{ "LAND", id_strategy_t::land, nullptr },
	{ "PGRD", id_strategy_t::pgrd, nullptr },
};

static constexpr id_rule_t default_id_rule = { "*", id_strategy_t::sub_text, "NAME" };

static const id_rule_t & find_id_rule(const std::string & rec_type)
{
	for (const auto & rule : id_rule_table)
	{
		if (rec_type == rule.rec_type)
			return rule;
	}

	return default_id_rule;
}

std::string plugin_index_t::derive_id(esm_reader_t & esm, size_t i)
{
	const auto & rec_type = esm.get_record().id;
	const auto & content = esm.get_record().content;
	const auto & rule = find_id_rule(rec_type);

	sub_record_iter_t iter(content);

	switch (rule.strategy)
	{
	case id_strategy_t::empty:
		return "";

	case id_strategy_t::cell:
		return derive_cell_id(iter, i);

	case id_strategy_t::index_based:
		return derive_index_based_id(iter, i);

	case id_strategy_t::script:
		return derive_script_id(iter, i);

	case id_strategy_t::sub_text:
		return derive_sub_text_id(iter, rule.sub_key, i);

	case id_strategy_t::land:
		return derive_land_id(iter, i);

	case id_strategy_t::pgrd:
		return derive_pgrd_id(iter, i);
	}

	return derive_sub_text_id(iter, "NAME", i);
}

enum class name_strategy_t
{
	index_named,
	sub_text,
	global_type
};

struct name_rule_t
{
	const char * rec_type;
	name_strategy_t strategy;
	const char * sub_key;
	const char * (*name_by_index)(int index);
};

static constexpr name_rule_t name_rule_table[] = {
	{ "MGEF", name_strategy_t::index_named, "INDX", effect_name_by_index },
	{ "SKIL", name_strategy_t::index_named, "INDX", skill_name_by_index },
	{ "INFO", name_strategy_t::sub_text, "ONAM", nullptr },
	{ "GLOB", name_strategy_t::global_type, "FNAM", nullptr },
};

static constexpr name_rule_t default_name_rule = { "*", name_strategy_t::sub_text, "FNAM", nullptr };

static const name_rule_t & find_name_rule(const std::string & rec_type)
{
	for (const auto & rule : name_rule_table)
	{
		if (rec_type == rule.rec_type)
			return rule;
	}

	return default_name_rule;
}

static std::string derive_index_named(const std::string & content, const name_rule_t & rule)
{
	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != rule.sub_key)
			continue;

		if (sub.size < indx_min_size)
			break;

		const int32_t index_val = static_cast<int32_t>(
		    domain_types::convert_string_byte_array_to_uint(std::string(sub.data, grid_coord_size)));

		const char * name = rule.name_by_index(index_val);
		if (name)
			return name;

		break;
	}

	return "";
}

static std::string derive_sub_text_name(const std::string & content, const name_rule_t & rule)
{
	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != rule.sub_key)
			continue;

		std::string text(sub.data, sub.size);
		return string_utils::erase_null_chars(text);
	}

	return "";
}

static std::string derive_global_type_name(const std::string & content, const name_rule_t & rule)
{
	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != rule.sub_key)
			continue;

		if (sub.size < 1)
			break;

		return global_type_name(sub.data[0]);
	}

	return "";
}

std::string plugin_index_t::derive_display_name(esm_reader_t & esm, size_t i)
{
	(void)i;
	const auto & content = esm.get_record().content;
	const auto & rec_type = esm.get_record().id;
	const auto & rule = find_name_rule(rec_type);

	switch (rule.strategy)
	{
	case name_strategy_t::index_named:
		return derive_index_named(content, rule);

	case name_strategy_t::sub_text:
		return derive_sub_text_name(content, rule);

	case name_strategy_t::global_type:
		return derive_global_type_name(content, rule);
	}

	return derive_sub_text_name(content, default_name_rule);
}
