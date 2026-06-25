#include "conflict_slots.hpp"
#include <unordered_map>
#include <set>
#include <cstring>

static void build_generic_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	struct sub_slot_t
	{
		std::string type;
		int occurrence;
	};

	std::vector<sub_slot_t> unified;

	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (const auto & sv : result.parsed[i])
		{
			int occ = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : unified)
			{
				if (slot.type == sv.type && slot.occurrence == occ)
				{
					found = true;
					break;
				}
			}

			if (!found)
				unified.push_back({ sv.type, occ });
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_type_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
			ver_type_indices[i][result.parsed[i][j].type].push_back(j);
	}

	result.aligned.reserve(unified.size());
	for (const auto & slot : unified)
	{
		aligned_slot_t as;
		as.key.type = slot.type;
		as.key.occurrence = slot.occurrence;
		as.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_type_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				as.indices[i] = SIZE_MAX;
			else
				as.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(as));
	}
}

static void build_leveled_list_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	struct lev_entry_t
	{
		std::string item_id;
		size_t intv_idx;
		size_t inam_idx;
	};

	std::vector<std::vector<lev_entry_t>> ver_entries(ver_count);
	std::vector<std::string> all_item_ids;

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = result.parsed[i];
		for (size_t j = 0; j + 1 < subs.size(); ++j)
		{
			if (subs[j].type != "INTV" || subs[j].size != 2)
				continue;

			if (subs[j + 1].type != "INAM")
				continue;

			std::string item_id(subs[j + 1].data, subs[j + 1].size);
			if (!item_id.empty() && item_id.back() == '\0')
				item_id.pop_back();

			ver_entries[i].push_back({ item_id, j, j + 1 });

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

	struct sub_slot_t
	{
		std::string type;
		int occurrence;
	};

	std::vector<sub_slot_t> header_slots;

	std::vector<std::set<size_t>> ver_paired_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & e : ver_entries[i])
		{
			ver_paired_indices[i].insert(e.intv_idx);
			ver_paired_indices[i].insert(e.inam_idx);
		}
	}

	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
		{
			if (ver_paired_indices[i].count(j))
				continue;

			const auto & sv = result.parsed[i][j];
			int occ = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : header_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occ)
				{
					found = true;
					break;
				}
			}

			if (!found)
				header_slots.push_back({ sv.type, occ });
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_type_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
		{
			if (ver_paired_indices[i].count(j))
				continue;

			ver_type_indices[i][result.parsed[i][j].type].push_back(j);
		}
	}

	for (const auto & slot : header_slots)
	{
		aligned_slot_t as;
		as.key.type = slot.type;
		as.key.occurrence = slot.occurrence;
		as.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_type_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				as.indices[i] = SIZE_MAX;
			else
				as.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(as));
	}

	int intv_occ = 0;
	int inam_occ = 0;
	for (const auto & s : header_slots)
	{
		if (s.type == "INTV")
			++intv_occ;

		if (s.type == "INAM")
			++inam_occ;
	}

	for (const auto & target_id : all_item_ids)
	{
		aligned_slot_t intv_slot;
		intv_slot.key.type = "INTV";
		intv_slot.key.occurrence = intv_occ++;
		intv_slot.indices.resize(ver_count);

		aligned_slot_t inam_slot;
		inam_slot.key.type = "INAM";
		inam_slot.key.occurrence = inam_occ++;
		inam_slot.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			bool matched = false;
			for (const auto & e : ver_entries[i])
			{
				if (e.item_id == target_id)
				{
					intv_slot.indices[i] = e.intv_idx;
					inam_slot.indices[i] = e.inam_idx;
					matched = true;
					break;
				}
			}

			if (!matched)
			{
				intv_slot.indices[i] = SIZE_MAX;
				inam_slot.indices[i] = SIZE_MAX;
			}
		}

		result.aligned.push_back(std::move(intv_slot));
		result.aligned.push_back(std::move(inam_slot));
	}
}

static void build_fact_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	struct fact_entry_t
	{
		std::string faction_name;
		size_t intv_idx;
		size_t anam_idx;
	};

	std::vector<std::vector<fact_entry_t>> ver_entries(ver_count);
	std::vector<std::string> all_faction_names;

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = result.parsed[i];
		for (size_t j = 0; j + 1 < subs.size(); ++j)
		{
			if (subs[j].type != "INTV" || subs[j].size != 4)
				continue;

			if (subs[j + 1].type != "ANAM")
				continue;

			std::string faction_name(subs[j + 1].data, subs[j + 1].size);
			if (!faction_name.empty() && faction_name.back() == '\0')
				faction_name.pop_back();

			ver_entries[i].push_back({ faction_name, j, j + 1 });

			bool found = false;
			for (const auto & name : all_faction_names)
			{
				if (name == faction_name)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_faction_names.push_back(faction_name);
		}
	}

	struct sub_slot_t
	{
		std::string type;
		int occurrence;
	};

	std::vector<sub_slot_t> header_slots;

	std::vector<std::set<size_t>> ver_paired_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & e : ver_entries[i])
		{
			ver_paired_indices[i].insert(e.intv_idx);
			ver_paired_indices[i].insert(e.anam_idx);
		}
	}

	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
		{
			if (ver_paired_indices[i].count(j))
				continue;

			const auto & sv = result.parsed[i][j];
			int occ = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : header_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occ)
				{
					found = true;
					break;
				}
			}

			if (!found)
				header_slots.push_back({ sv.type, occ });
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_type_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
		{
			if (ver_paired_indices[i].count(j))
				continue;

			ver_type_indices[i][result.parsed[i][j].type].push_back(j);
		}
	}

	for (const auto & slot : header_slots)
	{
		aligned_slot_t as;
		as.key.type = slot.type;
		as.key.occurrence = slot.occurrence;
		as.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_type_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				as.indices[i] = SIZE_MAX;
			else
				as.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(as));
	}

	int intv_occ = 0;
	int anam_occ = 0;
	for (const auto & s : header_slots)
	{
		if (s.type == "INTV")
			++intv_occ;

		if (s.type == "ANAM")
			++anam_occ;
	}

	for (const auto & target_name : all_faction_names)
	{
		aligned_slot_t intv_slot;
		intv_slot.key.type = "INTV";
		intv_slot.key.occurrence = intv_occ++;
		intv_slot.indices.resize(ver_count);

		aligned_slot_t anam_slot;
		anam_slot.key.type = "ANAM";
		anam_slot.key.occurrence = anam_occ++;
		anam_slot.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			bool matched = false;
			for (const auto & e : ver_entries[i])
			{
				if (e.faction_name == target_name)
				{
					intv_slot.indices[i] = e.intv_idx;
					anam_slot.indices[i] = e.anam_idx;
					matched = true;
					break;
				}
			}

			if (!matched)
			{
				intv_slot.indices[i] = SIZE_MAX;
				anam_slot.indices[i] = SIZE_MAX;
			}
		}

		result.aligned.push_back(std::move(intv_slot));
		result.aligned.push_back(std::move(anam_slot));
	}
}

static void build_container_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	struct cont_entry_t
	{
		std::string item_id;
		size_t npco_idx;
	};

	struct spell_entry_t
	{
		std::string spell_id;
		size_t npcs_idx;
	};

	std::vector<std::vector<cont_entry_t>> ver_items(ver_count);
	std::vector<std::string> all_item_ids;
	std::vector<std::vector<spell_entry_t>> ver_spells(ver_count);
	std::vector<std::string> all_spell_ids;

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & subs = result.parsed[i];
		for (size_t j = 0; j < subs.size(); ++j)
		{
			if (subs[j].type == "NPCO" && subs[j].size == 36)
			{
				std::string item_id(subs[j].data + 4, 32);
				while (!item_id.empty() && item_id.back() == '\0')
					item_id.pop_back();

				ver_items[i].push_back({ item_id, j });

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
			else if (subs[j].type == "NPCS" && subs[j].size == 32)
			{
				std::string spell_id(subs[j].data, 32);
				while (!spell_id.empty() && spell_id.back() == '\0')
					spell_id.pop_back();

				ver_spells[i].push_back({ spell_id, j });

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

	struct sub_slot_t
	{
		std::string type;
		int occurrence;
	};

	std::vector<sub_slot_t> header_slots;

	std::vector<std::set<size_t>> ver_paired_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & e : ver_items[i])
			ver_paired_indices[i].insert(e.npco_idx);

		for (const auto & e : ver_spells[i])
			ver_paired_indices[i].insert(e.npcs_idx);
	}

	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
		{
			if (ver_paired_indices[i].count(j))
				continue;

			const auto & sv = result.parsed[i][j];
			int occ = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : header_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occ)
				{
					found = true;
					break;
				}
			}

			if (!found)
				header_slots.push_back({ sv.type, occ });
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_type_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
		{
			if (ver_paired_indices[i].count(j))
				continue;

			ver_type_indices[i][result.parsed[i][j].type].push_back(j);
		}
	}

	for (const auto & slot : header_slots)
	{
		aligned_slot_t as;
		as.key.type = slot.type;
		as.key.occurrence = slot.occurrence;
		as.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_type_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				as.indices[i] = SIZE_MAX;
			else
				as.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(as));
	}

	int npco_occ = 0;
	for (const auto & s : header_slots)
	{
		if (s.type == "NPCO")
			++npco_occ;
	}

	for (const auto & target_id : all_item_ids)
	{
		aligned_slot_t npco_slot;
		npco_slot.key.type = "NPCO";
		npco_slot.key.occurrence = npco_occ++;
		npco_slot.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			bool matched = false;
			for (const auto & e : ver_items[i])
			{
				if (e.item_id == target_id)
				{
					npco_slot.indices[i] = e.npco_idx;
					matched = true;
					break;
				}
			}

			if (!matched)
				npco_slot.indices[i] = SIZE_MAX;
		}

		result.aligned.push_back(std::move(npco_slot));
	}

	int npcs_occ = 0;
	for (const auto & s : header_slots)
	{
		if (s.type == "NPCS")
			++npcs_occ;
	}

	for (const auto & target_id : all_spell_ids)
	{
		aligned_slot_t npcs_slot;
		npcs_slot.key.type = "NPCS";
		npcs_slot.key.occurrence = npcs_occ++;
		npcs_slot.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			bool matched = false;
			for (const auto & e : ver_spells[i])
			{
				if (e.spell_id == target_id)
				{
					npcs_slot.indices[i] = e.npcs_idx;
					matched = true;
					break;
				}
			}

			if (!matched)
				npcs_slot.indices[i] = SIZE_MAX;
		}

		result.aligned.push_back(std::move(npcs_slot));
	}
}

static void build_cell_slots(slot_result_t & result)
{
	const size_t ver_count = result.parsed.size();

	struct ref_group_t
	{
		uint32_t object_index;
		size_t start_idx;
		size_t end_idx;
	};

	std::vector<std::vector<ref_group_t>> ver_refs(ver_count);
	std::vector<uint32_t> all_object_indices;
	std::vector<size_t> ver_header_end(ver_count, 0);

	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < result.parsed[i].size(); ++j)
		{
			if (result.parsed[i][j].type != "FRMR")
				continue;

			if (ver_refs[i].empty())
				ver_header_end[i] = j;

			uint32_t obj_idx = 0;
			if (result.parsed[i][j].size >= 4)
				std::memcpy(&obj_idx, result.parsed[i][j].data, 4);

			size_t end = result.parsed[i].size();
			for (size_t k = j + 1; k < result.parsed[i].size(); ++k)
			{
				if (result.parsed[i][k].type == "FRMR")
				{
					end = k;
					break;
				}
			}

			ver_refs[i].push_back({ obj_idx, j, end });

			bool found = false;
			for (const auto & oi : all_object_indices)
			{
				if (oi == obj_idx)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_object_indices.push_back(obj_idx);
		}

		if (ver_refs[i].empty())
			ver_header_end[i] = result.parsed[i].size();
	}

	struct sub_slot_t
	{
		std::string type;
		int occurrence;
	};

	std::vector<sub_slot_t> header_slots;
	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (size_t j = 0; j < ver_header_end[i]; ++j)
		{
			const auto & sv = result.parsed[i][j];
			int occ = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : header_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occ)
				{
					found = true;
					break;
				}
			}

			if (!found)
				header_slots.push_back({ sv.type, occ });
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_header_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < ver_header_end[i]; ++j)
			ver_header_indices[i][result.parsed[i][j].type].push_back(j);
	}

	for (const auto & slot : header_slots)
	{
		aligned_slot_t as;
		as.key.type = slot.type;
		as.key.occurrence = slot.occurrence;
		as.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_header_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				as.indices[i] = SIZE_MAX;
			else
				as.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(as));
	}

	for (const auto & obj_idx : all_object_indices)
	{
		std::vector<sub_slot_t> ref_slots;
		for (size_t i = 0; i < ver_count; ++i)
		{
			for (const auto & ref : ver_refs[i])
			{
				if (ref.object_index != obj_idx)
					continue;

				std::unordered_map<std::string, int> type_count;
				for (size_t j = ref.start_idx; j < ref.end_idx; ++j)
				{
					const auto & sv = result.parsed[i][j];
					int occ = type_count[sv.type]++;
					bool found = false;
					for (const auto & slot : ref_slots)
					{
						if (slot.type == sv.type && slot.occurrence == occ)
						{
							found = true;
							break;
						}
					}

					if (!found)
						ref_slots.push_back({ sv.type, occ });
				}

				break;
			}
		}

		std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_ref_indices(ver_count);
		for (size_t i = 0; i < ver_count; ++i)
		{
			for (const auto & ref : ver_refs[i])
			{
				if (ref.object_index != obj_idx)
					continue;

				for (size_t j = ref.start_idx; j < ref.end_idx; ++j)
					ver_ref_indices[i][result.parsed[i][j].type].push_back(j);

				break;
			}
		}

		for (const auto & slot : ref_slots)
		{
			aligned_slot_t as;
			as.key.type = slot.type;
			as.key.occurrence = slot.occurrence;
			as.indices.resize(ver_count);

			for (size_t i = 0; i < ver_count; ++i)
			{
				auto & indices = ver_ref_indices[i][slot.type];
				if (slot.occurrence >= static_cast<int>(indices.size()))
					as.indices[i] = SIZE_MAX;
				else
					as.indices[i] = indices[slot.occurrence];
			}

			result.aligned.push_back(std::move(as));
		}
	}
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

	else if (rec_type == "CONT" || rec_type == "CREA" || rec_type == "NPC_")
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
