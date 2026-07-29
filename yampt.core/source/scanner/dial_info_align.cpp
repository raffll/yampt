#include "dial_info_align.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../utility/string_utils.hpp"
#include "plugin_scan.hpp"
#include <list>
#include <unordered_map>

struct info_insert_record_t
{
	std::string inam;
	std::string prev_inam;
	std::string display_name;
	int source_plugin_idx = -1;
};

static std::string extract_pnam(plugin_scan_t & scan, int plugin_idx, size_t record_index)
{
	const auto content = scan.read_record_content(plugin_idx, record_index);
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type == "PNAM")
			return string_utils::erase_null_chars(std::string(view.data, view.size));
	}

	return {};
}

static std::vector<info_align_entry_t> resolve_openmw_order(
    const std::vector<info_insert_record_t> & inserts,
    size_t plugin_count)
{
	struct ordered_item_t
	{
		std::string inam;
		std::string prev_inam;
		std::string display_name;
		int source_plugin_idx = -1;
		std::vector<bool> present_in_plugin;
	};

	std::list<ordered_item_t> ordered_list;
	std::unordered_map<std::string, std::list<ordered_item_t>::iterator> position_map;

	for (const auto & insert : inserts)
	{
		auto existing = position_map.find(insert.inam);

		if (existing != position_map.end())
		{
			if (existing->second->prev_inam == insert.prev_inam)
			{
				existing->second->display_name = insert.display_name;
				existing->second->source_plugin_idx = insert.source_plugin_idx;
				if (insert.source_plugin_idx >= 0)
					existing->second->present_in_plugin[insert.source_plugin_idx] = true;

				continue;
			}

			existing->second->display_name = insert.display_name;
			existing->second->prev_inam = insert.prev_inam;
			existing->second->source_plugin_idx = insert.source_plugin_idx;
			if (insert.source_plugin_idx >= 0)
				existing->second->present_in_plugin[insert.source_plugin_idx] = true;

			auto before = ordered_list.begin();
			if (!insert.prev_inam.empty())
			{
				auto prev_it = position_map.find(insert.prev_inam);
				if (prev_it != position_map.end())
					before = std::next(prev_it->second);
				else
					before = ordered_list.end();
			}

			ordered_list.splice(before, ordered_list, existing->second);
			continue;
		}

		auto before = ordered_list.begin();
		if (!insert.prev_inam.empty())
		{
			auto prev_it = position_map.find(insert.prev_inam);
			if (prev_it != position_map.end())
				before = std::next(prev_it->second);
			else
				before = ordered_list.end();
		}

		ordered_item_t item;
		item.inam = insert.inam;
		item.prev_inam = insert.prev_inam;
		item.display_name = insert.display_name;
		item.source_plugin_idx = insert.source_plugin_idx;
		item.present_in_plugin.resize(plugin_count, false);
		if (insert.source_plugin_idx >= 0)
			item.present_in_plugin[insert.source_plugin_idx] = true;

		auto inserted = ordered_list.insert(before, std::move(item));
		position_map[insert.inam] = inserted;
	}

	std::vector<info_align_entry_t> result;
	result.reserve(ordered_list.size());

	for (const auto & item : ordered_list)
	{
		info_align_entry_t entry;
		entry.inam = item.inam;
		entry.prev_inam = item.prev_inam;
		entry.display_name = item.display_name;
		entry.source_plugin_idx = item.source_plugin_idx;
		entry.present_in_plugin = item.present_in_plugin;
		result.push_back(std::move(entry));
	}

	return result;
}

dial_info_align_result_t dial_info_align_t::build(plugin_scan_t & scan, const std::string & dial_record_id)
{
	dial_info_align_result_t result;
	result.dial_record_id = dial_record_id;

	const size_t plugin_count = scan.plugin_count();
	for (size_t i = 0; i < plugin_count; ++i)
		result.plugin_names.push_back(scan.plugin_filename(static_cast<int>(i)));

	std::vector<info_insert_record_t> inserts;

	for (size_t plugin_idx = 0; plugin_idx < plugin_count; ++plugin_idx)
	{
		const auto & plugin_entries = scan.index(static_cast<int>(plugin_idx)).entries();

		for (size_t entry_idx = 0; entry_idx < plugin_entries.size(); ++entry_idx)
		{
			const auto & plugin_entry = plugin_entries[entry_idx];
			if (plugin_entry.rec_type != "INFO")
				continue;

			if (plugin_entry.dial_name != dial_record_id)
				continue;

			auto separator_pos = plugin_entry.record_id.find('|');
			const auto inam = (separator_pos != std::string::npos) ? plugin_entry.record_id.substr(separator_pos + 1)
			                                                       : plugin_entry.record_id;

			const auto prev_inam = extract_pnam(scan, static_cast<int>(plugin_idx), plugin_entry.record_index);

			info_insert_record_t insert;
			insert.inam = inam;
			insert.prev_inam = prev_inam;
			insert.display_name = plugin_entry.display_name;
			insert.source_plugin_idx = static_cast<int>(plugin_idx);
			inserts.push_back(std::move(insert));
		}
	}

	result.entries = resolve_openmw_order(inserts, plugin_count);
	return result;
}
