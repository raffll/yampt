#include "conflict_slots.hpp"
#include "conflict_slots_cell.hpp"
#include <cstring>
#include <set>
#include <unordered_map>

struct sub_slot_t
{
	std::string type;
	int occurrence;
};

static void collect_unified_slots(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::set<size_t>> & excluded,
    std::vector<sub_slot_t> & slots)
{
	const size_t ver_count = parsed.size();
	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (size_t j = 0; j < parsed[i].size(); ++j)
		{
			if (!excluded.empty() && excluded[i].count(j))
				continue;

			const auto & sv = parsed[i][j];
			int occ = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : slots)
			{
				if (slot.type == sv.type && slot.occurrence == occ)
				{
					found = true;
					break;
				}
			}

			if (!found)
				slots.push_back({ sv.type, occ });
		}
	}
}

static void align_slots_to_result(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::set<size_t>> & excluded,
    const std::vector<sub_slot_t> & slots,
    slot_result_t & result)
{
	const size_t ver_count = parsed.size();

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_type_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < parsed[i].size(); ++j)
		{
			if (!excluded.empty() && excluded[i].count(j))
				continue;

			ver_type_indices[i][parsed[i][j].type].push_back(j);
		}
	}

	for (const auto & slot : slots)
	{
		aligned_slot_t aligned;
		aligned.key.type = slot.type;
		aligned.key.occurrence = slot.occurrence;
		aligned.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_type_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				aligned.indices[i] = SIZE_MAX;
			else
				aligned.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(aligned));
	}
}

static int count_slot_occurrences(const std::vector<sub_slot_t> & slots, const std::string & target_type)
{
	int count = 0;
	for (const auto & slot : slots)
	{
		if (slot.type == target_type)
			++count;
	}
	return count;
}

static void build_generic_slots(slot_result_t & result)
{
	std::vector<sub_slot_t> unified;
	std::vector<std::set<size_t>> empty_excluded;
	collect_unified_slots(result.parsed, empty_excluded, unified);
	align_slots_to_result(result.parsed, empty_excluded, unified, result);
}

struct paired_entry_t
{
	std::string ident;
	size_t first_idx;
	size_t second_idx;
};

static std::string extract_null_terminated(const char * data, size_t size)
{
	std::string value(data, size);
	if (!value.empty() && value.back() == '\0')
		value.pop_back();

	return value;
}

static void collect_unique_ids(
    const std::vector<std::vector<paired_entry_t>> & ver_entries,
    std::vector<std::string> & all_ids)
{
	for (const auto & entries : ver_entries)
	{
		for (const auto & entry : entries)
		{
			bool found = false;
			for (const auto & id : all_ids)
			{
				if (id == entry.ident)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_ids.push_back(entry.ident);
		}
	}
}

static void build_paired_excluded(
    const std::vector<std::vector<paired_entry_t>> & ver_entries,
    size_t ver_count,
    std::vector<std::set<size_t>> & excluded)
{
	excluded.resize(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & entry : ver_entries[i])
		{
			excluded[i].insert(entry.first_idx);
			excluded[i].insert(entry.second_idx);
		}
	}
}

static void align_paired_entries(
    const std::vector<std::vector<paired_entry_t>> & ver_entries,
    const std::vector<std::string> & all_ids,
    const std::string & first_type,
    const std::string & second_type,
    int first_occ_start,
    int second_occ_start,
    slot_result_t & result)
{
	const size_t ver_count = ver_entries.size();
	int first_occ = first_occ_start;
	int second_occ = second_occ_start;

	for (const auto & target_id : all_ids)
	{
		aligned_slot_t first_slot;
		first_slot.key.type = first_type;
		first_slot.key.occurrence = first_occ++;
		first_slot.indices.resize(ver_count);

		aligned_slot_t second_slot;
		second_slot.key.type = second_type;
		second_slot.key.occurrence = second_occ++;
		second_slot.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			bool matched = false;
			for (const auto & entry : ver_entries[i])
			{
				if (entry.ident == target_id)
				{
					first_slot.indices[i] = entry.first_idx;
					second_slot.indices[i] = entry.second_idx;
					matched = true;
					break;
				}
			}

			if (!matched)
			{
				first_slot.indices[i] = SIZE_MAX;
				second_slot.indices[i] = SIZE_MAX;
			}
		}

		result.aligned.push_back(std::move(first_slot));
		result.aligned.push_back(std::move(second_slot));
	}
}

static void extract_lev_entries(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    std::vector<std::vector<paired_entry_t>> & ver_entries)
{
	const size_t ver_count = parsed.size();
	ver_entries.resize(ver_count);

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = parsed[i];
		for (size_t j = 0; j + 1 < subs.size(); ++j)
		{
			if (subs[j].type != "INTV" || subs[j].size != 2)
				continue;

			if (subs[j + 1].type != "INAM")
				continue;

			auto item_id = extract_null_terminated(subs[j + 1].data, subs[j + 1].size);
			ver_entries[i].push_back({ item_id, j, j + 1 });
		}
	}
}

static void build_leveled_list_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	std::vector<std::vector<paired_entry_t>> ver_entries;
	extract_lev_entries(result.parsed, ver_entries);

	std::vector<std::string> all_item_ids;
	collect_unique_ids(ver_entries, all_item_ids);

	std::vector<std::set<size_t>> excluded;
	build_paired_excluded(ver_entries, ver_count, excluded);

	std::vector<sub_slot_t> header_slots;
	collect_unified_slots(result.parsed, excluded, header_slots);
	align_slots_to_result(result.parsed, excluded, header_slots, result);

	const int intv_occ = count_slot_occurrences(header_slots, "INTV");
	const int inam_occ = count_slot_occurrences(header_slots, "INAM");

	align_paired_entries(ver_entries, all_item_ids, "INTV", "INAM", intv_occ, inam_occ, result);
}

static void extract_fact_entries(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    std::vector<std::vector<paired_entry_t>> & ver_entries)
{
	const size_t ver_count = parsed.size();
	ver_entries.resize(ver_count);

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = parsed[i];
		for (size_t j = 0; j + 1 < subs.size(); ++j)
		{
			if (subs[j].type != "INTV" || subs[j].size != 4)
				continue;

			if (subs[j + 1].type != "ANAM")
				continue;

			auto faction_name = extract_null_terminated(subs[j + 1].data, subs[j + 1].size);
			ver_entries[i].push_back({ faction_name, j, j + 1 });
		}
	}
}

static void build_fact_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	std::vector<std::vector<paired_entry_t>> ver_entries;
	extract_fact_entries(result.parsed, ver_entries);

	std::vector<std::string> all_faction_names;
	collect_unique_ids(ver_entries, all_faction_names);

	std::vector<std::set<size_t>> excluded;
	build_paired_excluded(ver_entries, ver_count, excluded);

	std::vector<sub_slot_t> header_slots;
	collect_unified_slots(result.parsed, excluded, header_slots);
	align_slots_to_result(result.parsed, excluded, header_slots, result);

	const int intv_occ = count_slot_occurrences(header_slots, "INTV");
	const int anam_occ = count_slot_occurrences(header_slots, "ANAM");

	align_paired_entries(ver_entries, all_faction_names, "INTV", "ANAM", intv_occ, anam_occ, result);
}

static constexpr size_t npco_record_size = 36;
static constexpr size_t npco_id_offset = 4;
static constexpr size_t npco_id_length = 32;
static constexpr size_t npcs_record_size = 32;

static void extract_npco_entries(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    std::vector<std::vector<paired_entry_t>> & ver_items,
    std::vector<std::string> & all_item_ids)
{
	const size_t ver_count = parsed.size();
	ver_items.resize(ver_count);

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = parsed[i];
		for (size_t j = 0; j < subs.size(); ++j)
		{
			if (subs[j].type != "NPCO" || subs[j].size != npco_record_size)
				continue;

			std::string item_id(subs[j].data + npco_id_offset, npco_id_length);
			while (!item_id.empty() && item_id.back() == '\0')
				item_id.pop_back();

			ver_items[i].push_back({ item_id, j, j });

			bool found = false;
			for (const auto & id : all_item_ids)
			{
				if (id == item_id)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_item_ids.push_back(item_id);
		}
	}
}

static void extract_npcs_entries(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    std::vector<std::vector<paired_entry_t>> & ver_spells,
    std::vector<std::string> & all_spell_ids)
{
	const size_t ver_count = parsed.size();
	ver_spells.resize(ver_count);

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = parsed[i];
		for (size_t j = 0; j < subs.size(); ++j)
		{
			if (subs[j].type != "NPCS" || subs[j].size != npcs_record_size)
				continue;

			std::string spell_id(subs[j].data, npcs_record_size);
			while (!spell_id.empty() && spell_id.back() == '\0')
				spell_id.pop_back();

			ver_spells[i].push_back({ spell_id, j, j });

			bool found = false;
			for (const auto & id : all_spell_ids)
			{
				if (id == spell_id)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_spell_ids.push_back(spell_id);
		}
	}
}

static void align_single_entries(
    const std::vector<std::vector<paired_entry_t>> & ver_entries,
    const std::vector<std::string> & all_ids,
    const std::string & slot_type,
    int occ_start,
    slot_result_t & result)
{
	const size_t ver_count = ver_entries.size();
	int occ = occ_start;

	for (const auto & target_id : all_ids)
	{
		aligned_slot_t aligned;
		aligned.key.type = slot_type;
		aligned.key.occurrence = occ++;
		aligned.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			bool matched = false;
			for (const auto & entry : ver_entries[i])
			{
				if (entry.ident == target_id)
				{
					aligned.indices[i] = entry.first_idx;
					matched = true;
					break;
				}
			}

			if (!matched)
				aligned.indices[i] = SIZE_MAX;
		}

		result.aligned.push_back(std::move(aligned));
	}
}

static void build_container_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	std::vector<std::vector<paired_entry_t>> ver_items;
	std::vector<std::string> all_item_ids;
	extract_npco_entries(result.parsed, ver_items, all_item_ids);

	std::vector<std::vector<paired_entry_t>> ver_spells;
	std::vector<std::string> all_spell_ids;
	extract_npcs_entries(result.parsed, ver_spells, all_spell_ids);

	std::vector<std::set<size_t>> excluded(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & entry : ver_items[i])
			excluded[i].insert(entry.first_idx);

		for (const auto & entry : ver_spells[i])
			excluded[i].insert(entry.first_idx);
	}

	std::vector<sub_slot_t> header_slots;
	collect_unified_slots(result.parsed, excluded, header_slots);
	align_slots_to_result(result.parsed, excluded, header_slots, result);

	const int npco_occ = count_slot_occurrences(header_slots, "NPCO");
	align_single_entries(ver_items, all_item_ids, "NPCO", npco_occ, result);

	const int npcs_occ = count_slot_occurrences(header_slots, "NPCS");
	align_single_entries(ver_spells, all_spell_ids, "NPCS", npcs_occ, result);
}

static void parse_versions(slot_result_t & result)
{
	size_t count = result.contents.size();
	result.parsed.resize(count);

	for (size_t i = 0; i < count; ++i)
	{
		sub_record_iter_t iter(result.contents[i]);
		sub_record_view_t sv;

		while (iter.next(sv))
			result.parsed[i].push_back(sv);
	}
}

static void dispatch_strategy(const std::string & rec_type, slot_result_t & result)
{
	if (rec_type == "CELL")
		build_cell_slots(result);

	else if (rec_type == "LEVI" || rec_type == "LEVC")
		build_leveled_list_slots(result);

	else if (rec_type == "FACT")
		build_fact_slots(result);

	else if (rec_type == "CONT" || rec_type == "CREA" || rec_type == "NPC_" || rec_type == "BSGN")
		build_container_slots(result);

	else
		build_generic_slots(result);
}

slot_result_t build_conflict_slots(
    const std::string & rec_type,
    const std::vector<std::string> & version_contents,
    const std::vector<bool> & version_deleted)
{
	slot_result_t result;
	result.contents = version_contents;
	result.is_deleted = version_deleted;

	parse_versions(result);
	dispatch_strategy(rec_type, result);

	return result;
}

slot_result_t build_conflict_slots(
    const std::string & rec_type,
    std::vector<std::string> && version_contents,
    const std::vector<bool> & version_deleted)
{
	slot_result_t result;
	result.contents = std::move(version_contents);
	result.is_deleted = version_deleted;

	parse_versions(result);
	dispatch_strategy(rec_type, result);

	return result;
}
