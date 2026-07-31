#include "conflict_slots.hpp"
#include <algorithm>
#include <cstring>
#include <set>
#include <unordered_map>

struct sub_slot_t
{
	std::string type;
	int occurrence;
};

static bool has_slot(const std::vector<sub_slot_t> & slots, const std::string & slot_type, int slot_occurrence)
{
	return std::any_of(
	    slots.begin(),
	    slots.end(),
	    [&](const sub_slot_t & slot) { return slot.type == slot_type && slot.occurrence == slot_occurrence; });
}

template<typename slot_container_t>
static bool has_slot_in(const slot_container_t & slots, const std::string & slot_type, int slot_occurrence)
{
	return std::any_of(
	    slots.begin(),
	    slots.end(),
	    [&](const auto & slot) { return slot.type == slot_type && slot.occurrence == slot_occurrence; });
}

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

			const auto & sub_view = parsed[i][j];
			int occurrence = type_count[sub_view.type]++;

			if (!has_slot(slots, sub_view.type, occurrence))
				slots.push_back({ sub_view.type, occurrence });
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
			const bool exists =
			    std::any_of(all_ids.begin(), all_ids.end(), [&](const std::string & id) { return id == entry.ident; });

			if (!exists)
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
    const std::string & id_sub_type,
    std::vector<std::vector<paired_entry_t>> & ver_entries)
{
	const size_t ver_count = parsed.size();
	ver_entries.resize(ver_count);

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = parsed[i];
		for (size_t j = 0; j + 1 < subs.size(); ++j)
		{
			if (subs[j].type != id_sub_type)
				continue;

			if (subs[j + 1].type != "INTV" || subs[j + 1].size != 2)
				continue;

			auto item_id = extract_null_terminated(subs[j].data, subs[j].size);
			ver_entries[i].push_back({ item_id, j, j + 1 });
		}
	}
}

static void build_leveled_list_slots(const std::string & rec_type, slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();
	const std::string id_sub_type = (rec_type == "LEVC") ? "CNAM" : "INAM";

	std::vector<std::vector<paired_entry_t>> ver_entries;
	extract_lev_entries(result.parsed, id_sub_type, ver_entries);

	std::vector<std::string> all_item_ids;
	collect_unique_ids(ver_entries, all_item_ids);

	std::vector<std::set<size_t>> excluded;
	build_paired_excluded(ver_entries, ver_count, excluded);

	std::vector<sub_slot_t> header_slots;
	collect_unified_slots(result.parsed, excluded, header_slots);
	align_slots_to_result(result.parsed, excluded, header_slots, result);

	const int id_occ = count_slot_occurrences(header_slots, id_sub_type);
	const int intv_occ = count_slot_occurrences(header_slots, "INTV");

	align_paired_entries(ver_entries, all_item_ids, id_sub_type, "INTV", id_occ, intv_occ, result);
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

			const bool exists = std::any_of(
			    all_item_ids.begin(), all_item_ids.end(), [&](const std::string & id) { return id == item_id; });

			if (!exists)
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

			const bool exists = std::any_of(
			    all_spell_ids.begin(), all_spell_ids.end(), [&](const std::string & id) { return id == spell_id; });

			if (!exists)
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
	int occurrence = occ_start;

	for (const auto & target_id : all_ids)
	{
		aligned_slot_t aligned;
		aligned.key.type = slot_type;
		aligned.key.occurrence = occurrence++;
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
		sub_record_view_t sub_view;

		while (iter.next(sub_view))
			result.parsed[i].push_back(sub_view);
	}
}

struct armor_group_entry_t
{
	std::string indx_key;
	size_t indx_idx;
	size_t bnam_idx;
	size_t cnam_idx;
};

static void extract_armor_groups(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    std::vector<std::vector<armor_group_entry_t>> & ver_groups,
    std::vector<std::string> & all_indx_keys)
{
	const size_t ver_count = parsed.size();
	ver_groups.resize(ver_count);

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = parsed[i];
		for (size_t j = 0; j < subs.size(); ++j)
		{
			if (subs[j].type != "INDX" || subs[j].size < 4)
				continue;

			bool has_bnam_or_cnam_after =
			    (j + 1 < subs.size()) && (subs[j + 1].type == "BNAM" || subs[j + 1].type == "CNAM");

			if (!has_bnam_or_cnam_after)
				continue;

			uint32_t indx_value = 0;
			std::memcpy(&indx_value, subs[j].data, 4);
			auto indx_key = std::to_string(indx_value);

			size_t bnam_idx = SIZE_MAX;
			size_t cnam_idx = SIZE_MAX;

			size_t next = j + 1;
			while (next < subs.size() && (subs[next].type == "BNAM" || subs[next].type == "CNAM"))
			{
				if (subs[next].type == "BNAM" && bnam_idx == SIZE_MAX)
					bnam_idx = next;
				else if (subs[next].type == "CNAM" && cnam_idx == SIZE_MAX)
					cnam_idx = next;

				++next;
			}

			ver_groups[i].push_back({ indx_key, j, bnam_idx, cnam_idx });

			const bool exists = std::any_of(
			    all_indx_keys.begin(), all_indx_keys.end(), [&](const std::string & key) { return key == indx_key; });

			if (!exists)
				all_indx_keys.push_back(indx_key);
		}
	}
}

static void build_armor_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	std::vector<std::vector<armor_group_entry_t>> ver_groups;
	std::vector<std::string> all_indx_keys;
	extract_armor_groups(result.parsed, ver_groups, all_indx_keys);

	std::vector<std::set<size_t>> excluded(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & group : ver_groups[i])
		{
			excluded[i].insert(group.indx_idx);
			if (group.bnam_idx != SIZE_MAX)
				excluded[i].insert(group.bnam_idx);
			if (group.cnam_idx != SIZE_MAX)
				excluded[i].insert(group.cnam_idx);
		}
	}

	std::vector<sub_slot_t> header_slots;
	collect_unified_slots(result.parsed, excluded, header_slots);
	align_slots_to_result(result.parsed, excluded, header_slots, result);

	const int indx_occ_start = count_slot_occurrences(header_slots, "INDX");
	const int bnam_occ_start = count_slot_occurrences(header_slots, "BNAM");
	const int cnam_occ_start = count_slot_occurrences(header_slots, "CNAM");

	int indx_occ = indx_occ_start;
	int bnam_occ = bnam_occ_start;
	int cnam_occ = cnam_occ_start;

	for (const auto & target_key : all_indx_keys)
	{
		aligned_slot_t indx_slot;
		indx_slot.key.type = "INDX";
		indx_slot.key.occurrence = indx_occ++;
		indx_slot.indices.resize(ver_count);

		aligned_slot_t bnam_slot;
		bnam_slot.key.type = "BNAM";
		bnam_slot.key.occurrence = bnam_occ++;
		bnam_slot.indices.resize(ver_count);

		aligned_slot_t cnam_slot;
		cnam_slot.key.type = "CNAM";
		cnam_slot.key.occurrence = cnam_occ++;
		cnam_slot.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			const auto it_group = std::find_if(
			    ver_groups[i].begin(),
			    ver_groups[i].end(),
			    [&](const armor_group_entry_t & group) { return group.indx_key == target_key; });

			if (it_group != ver_groups[i].end())
			{
				indx_slot.indices[i] = it_group->indx_idx;
				bnam_slot.indices[i] = it_group->bnam_idx;
				cnam_slot.indices[i] = it_group->cnam_idx;
			}
			else
			{
				indx_slot.indices[i] = SIZE_MAX;
				bnam_slot.indices[i] = SIZE_MAX;
				cnam_slot.indices[i] = SIZE_MAX;
			}
		}

		result.aligned.push_back(std::move(indx_slot));
		result.aligned.push_back(std::move(bnam_slot));
		result.aligned.push_back(std::move(cnam_slot));
	}
}

static void dispatch_strategy(const std::string & rec_type, slot_result_t & result)
{
	if (rec_type == "CELL")
		conflict_slots::build_cell(result);

	else if (rec_type == "LEVI" || rec_type == "LEVC")
		build_leveled_list_slots(rec_type, result);

	else if (rec_type == "FACT")
		build_fact_slots(result);

	else if (rec_type == "CONT" || rec_type == "CREA" || rec_type == "NPC_" || rec_type == "BSGN" || rec_type == "RACE")
		build_container_slots(result);

	else if (rec_type == "ARMO" || rec_type == "CLOT")
		build_armor_slots(result);

	else
		build_generic_slots(result);
}

slot_result_t conflict_slots::build(
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

slot_result_t conflict_slots::build(
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

// Cell slot building

struct cell_sub_slot_t
{
	std::string type;
	int occurrence;
};

struct cell_ref_group_t
{
	uint32_t object_index;
	size_t start_idx;
	size_t end_idx;
};

static void extract_cell_refs(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    std::vector<size_t> & ver_header_end,
    std::vector<uint32_t> & all_object_indices)
{
	const size_t ver_count = parsed.size();
	ver_refs.resize(ver_count);
	ver_header_end.resize(ver_count, 0);

	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < parsed[i].size(); ++j)
		{
			if (parsed[i][j].type != "FRMR")
				continue;

			if (ver_refs[i].empty())
				ver_header_end[i] = j;

			uint32_t obj_idx = read_frmr_ref_index(parsed[i][j].data, parsed[i][j].size);

			size_t end_pos = parsed[i].size();
			for (size_t k = j + 1; k < parsed[i].size(); ++k)
			{
				if (parsed[i][k].type == "FRMR")
				{
					end_pos = k;
					break;
				}
			}

			ver_refs[i].push_back({ obj_idx, j, end_pos });

			const bool exists = std::any_of(
			    all_object_indices.begin(), all_object_indices.end(), [&](uint32_t index) { return index == obj_idx; });

			if (!exists)
				all_object_indices.push_back(obj_idx);
		}

		if (ver_refs[i].empty())
			ver_header_end[i] = parsed[i].size();
	}
}

static void align_cell_header(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<size_t> & ver_header_end,
    slot_result_t & result)
{
	const size_t ver_count = parsed.size();

	std::vector<cell_sub_slot_t> header_slots;
	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (size_t j = 0; j < ver_header_end[i]; ++j)
		{
			const auto & sub_view = parsed[i][j];
			int occurrence = type_count[sub_view.type]++;

			if (!has_slot_in(header_slots, sub_view.type, occurrence))
				header_slots.push_back({ sub_view.type, occurrence });
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_header_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < ver_header_end[i]; ++j)
			ver_header_indices[i][parsed[i][j].type].push_back(j);
	}

	for (const auto & slot : header_slots)
	{
		aligned_slot_t aligned;
		aligned.key.type = slot.type;
		aligned.key.occurrence = slot.occurrence;
		aligned.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_header_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				aligned.indices[i] = SIZE_MAX;
			else
				aligned.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(aligned));
	}
}

static void collect_ref_group_slots(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    uint32_t object_index,
    std::vector<cell_sub_slot_t> & ref_slots)
{
	const size_t ver_count = parsed.size();
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & ref : ver_refs[i])
		{
			if (ref.object_index != object_index)
				continue;

			std::unordered_map<std::string, int> type_count;
			const size_t safe_end = std::min(ref.end_idx, parsed[i].size());
			for (size_t j = ref.start_idx; j < safe_end; ++j)
			{
				const auto & sub_view = parsed[i][j];
				int occurrence = type_count[sub_view.type]++;

				if (!has_slot_in(ref_slots, sub_view.type, occurrence))
					ref_slots.push_back({ sub_view.type, occurrence });
			}

			break;
		}
	}
}

static void align_ref_group_slots(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    uint32_t object_index,
    const std::vector<cell_sub_slot_t> & ref_slots,
    slot_result_t & result)
{
	const size_t ver_count = parsed.size();

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_ref_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & ref : ver_refs[i])
		{
			if (ref.object_index != object_index)
				continue;

			const size_t safe_end = std::min(ref.end_idx, parsed[i].size());
			for (size_t j = ref.start_idx; j < safe_end; ++j)
				ver_ref_indices[i][parsed[i][j].type].push_back(j);

			break;
		}
	}

	for (const auto & slot : ref_slots)
	{
		aligned_slot_t aligned;
		aligned.key.type = slot.type;
		aligned.key.occurrence = slot.occurrence;
		aligned.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_ref_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				aligned.indices[i] = SIZE_MAX;
			else
				aligned.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(aligned));
	}
}

static void align_cell_ref_group(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    uint32_t object_index,
    slot_result_t & result)
{
	std::vector<cell_sub_slot_t> ref_slots;
	collect_ref_group_slots(parsed, ver_refs, object_index, ref_slots);
	align_ref_group_slots(parsed, ver_refs, object_index, ref_slots, result);
}

void conflict_slots::build_cell(slot_result_t & result)
{
	std::vector<std::vector<cell_ref_group_t>> ver_refs;
	std::vector<size_t> ver_header_end;
	std::vector<uint32_t> all_object_indices;
	extract_cell_refs(result.parsed, ver_refs, ver_header_end, all_object_indices);

	align_cell_header(result.parsed, ver_header_end, result);

	for (const auto & obj_idx : all_object_indices)
		align_cell_ref_group(result.parsed, ver_refs, obj_idx, result);
}
