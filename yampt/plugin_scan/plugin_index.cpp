#include "plugin_index.hpp"
#include "sub_record_iter.hpp"
#include <algorithm>
#include <set>

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
			entry.dial_name = current_dial;

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
		if (lookup_.find(key) == lookup_.end())
			lookup_[key] = entries_.size();

		entries_.push_back(std::move(entry));
	}
}

const std::vector<indexed_record_t> & plugin_index_t::entries() const
{
	return entries_;
}

const indexed_record_t * plugin_index_t::find(const std::string & type, const std::string & id) const
{
	std::string key = type + std::string(1, '\0') + id;
	auto it = lookup_.find(key);
	if (it == lookup_.end())
		return nullptr;

	return &entries_[it->second];
}

std::vector<std::string> plugin_index_t::types() const
{
	std::set<std::string> unique;
	for (const auto & entry : entries_)
		unique.insert(entry.rec_type);

	return std::vector<std::string>(unique.begin(), unique.end());
}

size_t plugin_index_t::count_by_type(const std::string & type) const
{
	size_t count = 0;
	for (const auto & entry : entries_)
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
	sub_record_view_t sub;

	if (rec_type == "CELL")
	{
		std::string name_text;
		bool has_name = false;
		int32_t grid_x = 0;
		int32_t grid_y = 0;
		bool is_interior = false;
		bool has_data = false;

		while (iter.next(sub))
		{
			if (sub.type == "NAME")
			{
				name_text = std::string(sub.data, sub.size);
				name_text = tools_t::erase_null_chars(name_text);
				has_name = true;
				continue;
			}

			if (sub.type == "DATA" && sub.size >= 12)
			{
				uint32_t flags = tools_t::convert_string_byte_array_to_uint(
				    std::string(sub.data, 4));
				is_interior = (flags & 0x01) != 0;
				grid_x = static_cast<int32_t>(tools_t::convert_string_byte_array_to_uint(
				    std::string(sub.data + 4, 4)));
				grid_y = static_cast<int32_t>(tools_t::convert_string_byte_array_to_uint(
				    std::string(sub.data + 8, 4)));
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

		return std::to_string(i);
	}

	if (rec_type == "SKIL" || rec_type == "MGEF")
	{
		while (iter.next(sub))
		{
			if (sub.type != "INDX")
				continue;

			if (sub.size < 4)
				break;

			int32_t index_val = static_cast<int32_t>(
			    tools_t::convert_string_byte_array_to_uint(std::string(sub.data, 4)));
			return std::to_string(index_val);
		}

		return std::to_string(i);
	}

	if (rec_type == "SCPT")
	{
		while (iter.next(sub))
		{
			if (sub.type != "SCHD")
				continue;

			size_t name_len = std::min(sub.size, static_cast<size_t>(32));
			std::string script_name(sub.data, name_len);
			script_name = tools_t::erase_null_chars(script_name);
			return script_name;
		}

		return std::to_string(i);
	}

	if (rec_type == "DIAL")
	{
		while (iter.next(sub))
		{
			if (sub.type != "NAME")
				continue;

			std::string text(sub.data, sub.size);
			text = tools_t::erase_null_chars(text);
			return text;
		}

		return std::to_string(i);
	}

	if (rec_type == "INFO")
	{
		while (iter.next(sub))
		{
			if (sub.type != "INAM")
				continue;

			std::string text(sub.data, sub.size);
			text = tools_t::erase_null_chars(text);
			return text;
		}

		return std::to_string(i);
	}

	if (rec_type == "LAND")
	{
		while (iter.next(sub))
		{
			if (sub.type != "INTV")
				continue;

			if (sub.size < 8)
				break;

			int32_t grid_x = static_cast<int32_t>(
			    tools_t::convert_string_byte_array_to_uint(std::string(sub.data, 4)));
			int32_t grid_y = static_cast<int32_t>(
			    tools_t::convert_string_byte_array_to_uint(std::string(sub.data + 4, 4)));
			return "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";
		}

		return std::to_string(i);
	}

	if (rec_type == "PGRD")
	{
		while (iter.next(sub))
		{
			if (sub.type != "DATA")
				continue;

			if (sub.size < 8)
				break;

			int32_t grid_x = static_cast<int32_t>(
			    tools_t::convert_string_byte_array_to_uint(std::string(sub.data, 4)));
			int32_t grid_y = static_cast<int32_t>(
			    tools_t::convert_string_byte_array_to_uint(std::string(sub.data + 4, 4)));
			return "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";
		}

		return std::to_string(i);
	}

	while (iter.next(sub))
	{
		if (sub.type != "NAME")
			continue;

		std::string text(sub.data, sub.size);
		text = tools_t::erase_null_chars(text);
		return text;
	}

	return std::to_string(i);
}

std::string plugin_index_t::derive_display_name(esm_reader_t & esm, size_t i)
{
	const auto & content = esm.get_record().content;

	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	while (iter.next(sub))
	{
		if (sub.type != "FNAM")
			continue;

		std::string text(sub.data, sub.size);
		text = tools_t::erase_null_chars(text);
		return text;
	}

	return "";
}
