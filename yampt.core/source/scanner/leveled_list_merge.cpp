#include "leveled_list_merge.hpp"
#include "../decoder/record_header_flags.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../decoder/sub_record_schema.hpp"
#include "../utility/app_logger.hpp"
#include "../utility/domain_types.hpp"
#include "../utility/record_behavior.hpp"
#include "../utility/string_utils.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <optional>
#include <set>

static constexpr size_t leveled_level_size = leveled_layout::level_size;

struct list_item_t
{
	std::string ident;
	uint16_t level;
};

static std::vector<list_item_t> extract_list_items(const std::string & content, const std::string & rec_type)
{
	std::vector<list_item_t> items;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	std::string current_id;

	const std::string item_sub_type = leveled_item_sub_type_for(rec_type);
	const std::string level_sub_type = leveled_level_sub_type_for(rec_type);

	while (iter.next(sub))
	{
		if (sub.type == item_sub_type)
		{
			current_id = std::string(sub.data, sub.size);
			current_id = string_utils::erase_null_chars(current_id);
			continue;
		}

		if (sub.type == level_sub_type && !current_id.empty())
		{
			uint16_t level = 0;
			if (sub.size >= leveled_level_size)
				std::memcpy(&level, sub.data, leveled_level_size);

			items.push_back({ current_id, level });
			current_id.clear();
		}
	}

	return items;
}

static sub_record_sequence_t extract_header_subs(const std::string & content, const std::string & rec_type)
{
	sub_record_sequence_t header;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;

	const std::string item_sub_type = leveled_item_sub_type_for(rec_type);
	const std::string level_sub_type = leveled_level_sub_type_for(rec_type);

	while (iter.next(sub))
	{
		if (sub.type == item_sub_type || sub.type == level_sub_type)
			break;

		if (sub.type == "INDX")
			continue;

		header.push_back({ sub.type, std::string(sub.data, sub.size) });
	}

	return header;
}

static std::string merge_header_part(const std::vector<std::string> & versions, const std::string & rec_type)
{
	const auto first_header = extract_header_subs(versions.front(), rec_type);
	const auto winner_header = extract_header_subs(versions.back(), rec_type);
	auto output = winner_header;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const auto inter_header = extract_header_subs(versions[version_idx], rec_type);
		sub_record_merge_t::apply_intermediate(output, first_header, inter_header, winner_header, rec_type);
	}

	std::string header_part;
	for (const auto & entry : output)
		header_part += sub_record_merge_t::serialize_sub_record(entry);

	return header_part;
}

static std::string build_merged_list_record(
    const std::string & rec_type,
    const std::string & header_part,
    const std::vector<list_item_t> & merged_items)
{
	std::string indx_sub = "INDX";
	uint32_t item_count = static_cast<uint32_t>(merged_items.size());
	indx_sub += domain_types::convert_uint_to_string_byte_array(4);
	indx_sub += std::string(reinterpret_cast<const char *>(&item_count), 4);

	const char * const item_sub_type_name = leveled_item_sub_type_for(rec_type);
	const char * const level_sub_type_name = leveled_level_sub_type_for(rec_type);
	if (item_sub_type_name == nullptr || level_sub_type_name == nullptr)
	{
		app_logger_t::add_log("[error] no leveled item sub-type for " + rec_type + "\r\n", true);
		return {};
	}

	const std::string item_sub_type = item_sub_type_name;
	const std::string level_sub_type = level_sub_type_name;
	std::string items_part;
	for (const auto & item : merged_items)
	{
		std::string id_data = item.ident;
		id_data.push_back('\0');

		items_part += item_sub_type;
		items_part += domain_types::convert_uint_to_string_byte_array(id_data.size());
		items_part += id_data;

		items_part += level_sub_type;
		items_part += domain_types::convert_uint_to_string_byte_array(leveled_level_size);
		items_part += std::string(reinterpret_cast<const char *>(&item.level), leveled_level_size);
	}

	std::string body = header_part + indx_sub + items_part;

	std::string record;
	record += rec_type;
	record += domain_types::convert_uint_to_string_byte_array(body.size());
	record += std::string(8, '\0');
	record += body;

	return record;
}

using item_levels_map_t = std::map<std::string, std::vector<uint16_t>>;

static item_levels_map_t build_item_levels_map(const std::vector<list_item_t> & items)
{
	item_levels_map_t levels;
	for (const auto & item : items)
		levels[item.ident].push_back(item.level);

	return levels;
}

static bool is_item_deleted(
    const std::string & ident,
    const item_levels_map_t & first_map,
    const std::vector<item_levels_map_t> & plugin_maps)
{
	const auto it_first = first_map.find(ident);
	if (it_first == first_map.end() || it_first->second.empty())
		return false;

	for (const auto & version_map : plugin_maps)
	{
		if (version_map.find(ident) == version_map.end())
			return true;
	}

	return false;
}

static size_t version_count(const item_levels_map_t & map, const std::string & ident)
{
	const auto it_map = map.find(ident);
	return it_map == map.end() ? 0 : it_map->second.size();
}

struct count_context_t
{
	const std::string & ident;
	const item_levels_map_t & first_map;
	const std::vector<item_levels_map_t> & plugin_maps;
};

static size_t merged_occurrence_count(const count_context_t & context)
{
	const size_t master_count = version_count(context.first_map, context.ident);

	size_t resolved = master_count;
	for (const auto & version_map : context.plugin_maps)
	{
		const size_t plugin_count = version_count(version_map, context.ident);
		if (plugin_count != master_count)
			resolved = plugin_count;
	}

	return resolved;
}

struct occurrence_context_t
{
	const std::string & ident;
	size_t occurrence;
	const item_levels_map_t & first_map;
	const std::vector<item_levels_map_t> & plugin_maps;
};

static std::optional<uint16_t> level_at(const item_levels_map_t & map, const std::string & ident, size_t occurrence)
{
	const auto it_map = map.find(ident);
	if (it_map == map.end() || occurrence >= it_map->second.size())
		return std::nullopt;

	return it_map->second[occurrence];
}

static uint16_t resolve_occurrence_level(const occurrence_context_t & context)
{
	const auto master_level = level_at(context.first_map, context.ident, context.occurrence);

	uint16_t resolved = master_level.value_or(0);
	bool has_resolved = master_level.has_value();

	for (const auto & version_map : context.plugin_maps)
	{
		const auto plugin_level = level_at(version_map, context.ident, context.occurrence);
		if (!plugin_level.has_value())
			continue;

		if (!master_level.has_value() || *plugin_level != *master_level)
		{
			resolved = *plugin_level;
			has_resolved = true;
		}
	}

	return has_resolved ? resolved : master_level.value_or(0);
}

static std::vector<list_item_t> build_merged_items(
    const item_levels_map_t & first_map,
    const std::vector<item_levels_map_t> & plugin_maps)
{
	std::set<std::string> all_idents;
	for (const auto & [ident, levels] : first_map)
		all_idents.insert(ident);

	for (const auto & version_map : plugin_maps)
	{
		for (const auto & [ident, levels] : version_map)
			all_idents.insert(ident);
	}

	std::vector<list_item_t> merged;
	for (const auto & ident : all_idents)
	{
		if (is_item_deleted(ident, first_map, plugin_maps))
			continue;

		const count_context_t count_context { ident, first_map, plugin_maps };
		const auto count = merged_occurrence_count(count_context);
		for (size_t occurrence = 0; occurrence < count; ++occurrence)
		{
			const occurrence_context_t context { ident, occurrence, first_map, plugin_maps };
			merged.push_back({ ident, resolve_occurrence_level(context) });
		}
	}

	return merged;
}

static void sort_merged_items(std::vector<list_item_t> & items)
{
	std::sort(
	    items.begin(),
	    items.end(),
	    [](const list_item_t & left, const list_item_t & right)
	{
		if (left.level != right.level)
			return left.level < right.level;

		return left.ident < right.ident;
	});
}

merge_result_t leveled_list_merge_t::merge(const merge_input_t & input)
{
	const auto & versions = input.version_contents;
	if (versions.size() < 2)
		return { false, {} };

	const auto & first_content = versions.front();
	auto master_items = extract_list_items(first_content, input.rec_type);
	const auto first_map = build_item_levels_map(master_items);

	std::vector<item_levels_map_t> plugin_maps;

	for (size_t vi = 1; vi < versions.size(); ++vi)
		plugin_maps.push_back(build_item_levels_map(extract_list_items(versions[vi], input.rec_type)));

	if (plugin_maps.empty())
		return { false, {} };

	auto merged = build_merged_items(first_map, plugin_maps);
	sort_merged_items(merged);

	const auto header_part = merge_header_part(versions, input.rec_type);
	auto record = build_merged_list_record(input.rec_type, header_part, merged);
	record_header_flags::write_flags(record, record_header_flags::merge_flags(versions));

	sort_merged_items(master_items);
	const auto master_header_part = merge_header_part({ first_content, first_content }, input.rec_type);
	auto master_record = build_merged_list_record(input.rec_type, master_header_part, master_items);
	record_header_flags::write_flags(master_record, record_header_flags::read_flags(first_content));

	if (record == master_record)
		return { false, first_content };

	return { true, record };
}
