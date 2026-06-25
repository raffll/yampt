#include "plugin_scan.hpp"
#include "conflict_compute.hpp"
#include "sub_record_iter.hpp"
#include "sub_record_schema.hpp"
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <cstring>

void plugin_scan_t::load_plugin(const std::string & path)
{
	auto p = std::make_unique<loaded_plugin_t>(path);
	plugins_.push_back(std::move(p));
}

void plugin_scan_t::set_merge_plugin(const std::string & filename)
{
	merge_plugin_idx_ = static_cast<int>(plugins_.size());

	auto p = std::make_unique<loaded_plugin_t>();
	p->path = filename;
	plugins_.push_back(std::move(p));

	merge_records_.clear();
}

void plugin_scan_t::rebuild_conflicts()
{
	entries_.clear();
	entry_lookup_.clear();

	for (int pi = 0; pi < static_cast<int>(plugins_.size()); ++pi)
	{
		if (pi == merge_plugin_idx_)
			continue;

		const auto & idx = plugins_[pi]->index;
		for (const auto & rec : idx.entries())
		{
			if (rec.rec_type == "TES3")
				continue;

			std::string key = rec.rec_type + std::string(1, '\0') + rec.record_id;
			auto it = entry_lookup_.find(key);

			if (it == entry_lookup_.end())
			{
				conflict_entry_t entry;
				entry.rec_type = rec.rec_type;
				entry.record_id = rec.record_id;
				entry.display_name = rec.display_name;
				entry.dial_name = rec.dial_name;
				entry.conflict_all = conflict_all_t::only_one;

				record_version_t ver;
				ver.plugin_idx = pi;
				ver.record_index = rec.record_index;
				ver.status = conflict_this_t::master;
				entry.versions.push_back(ver);

				entry_lookup_[key] = entries_.size();
				entries_.push_back(std::move(entry));
			}
			else
			{
				auto & entry = entries_[it->second];

				record_version_t ver;
				ver.plugin_idx = pi;
				ver.record_index = rec.record_index;
				ver.status = conflict_this_t::unknown;
				entry.versions.push_back(ver);

				if (entry.display_name.empty() && !rec.display_name.empty())
					entry.display_name = rec.display_name;
			}
		}
	}

	if (merge_plugin_idx_ >= 0)
	{
		for (size_t mi = 0; mi < merge_records_.size(); ++mi)
		{
			const auto & mr = merge_records_[mi];
			std::string key = mr.rec_type + std::string(1, '\0') + mr.record_id;
			auto it = entry_lookup_.find(key);

			if (it == entry_lookup_.end())
			{
				conflict_entry_t entry;
				entry.rec_type = mr.rec_type;
				entry.record_id = mr.record_id;
				entry.conflict_all = conflict_all_t::only_one;

				record_version_t ver;
				ver.plugin_idx = merge_plugin_idx_;
				ver.record_index = mi;
				ver.status = conflict_this_t::master;
				entry.versions.push_back(ver);

				entry_lookup_[key] = entries_.size();
				entries_.push_back(std::move(entry));
			}
			else
			{
				auto & entry = entries_[it->second];

				record_version_t ver;
				ver.plugin_idx = merge_plugin_idx_;
				ver.record_index = mi;
				ver.status = conflict_this_t::unknown;
				entry.versions.push_back(ver);
			}
		}
	}

	for (auto & entry : entries_)
	{
		if (entry.versions.size() >= 2)
			compute_conflict(entry);
	}
}

void plugin_scan_t::compute_conflict(conflict_entry_t & entry)
{
	const size_t ver_count = entry.versions.size();

	std::vector<std::string> contents(ver_count);
	std::vector<bool> is_deleted(ver_count, false);

	for (size_t i = 0; i < ver_count; ++i)
	{
		auto & ver = entry.versions[i];

		if (ver.plugin_idx == merge_plugin_idx_)
		{
			contents[i] = merge_records_[ver.record_index].content;
		}
		else
		{
			plugins_[ver.plugin_idx]->esm.select_record(ver.record_index);
			contents[i] = plugins_[ver.plugin_idx]->esm.get_record().content;
		}

		if (ver.plugin_idx != merge_plugin_idx_)
		{
			const auto & plugin_entries = plugins_[ver.plugin_idx]->index.entries();
			if (ver.record_index < plugin_entries.size() && plugin_entries[ver.record_index].has_dele)
				is_deleted[i] = true;
		}
	}

	using slot_key_t = std::pair<std::string, int>;
	std::vector<slot_key_t> unified_slots;

	std::vector<std::vector<sub_record_view_t>> parsed(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		if (contents[i].size() < 16)
			continue;

		sub_record_iter_t iter(contents[i]);
		sub_record_view_t sv;
		std::map<std::string, int> occurrence_count;

		while (iter.next(sv))
		{
			int occ = occurrence_count[sv.type]++;
			sv.offset = static_cast<size_t>(occ);
			parsed[i].push_back(sv);
		}
	}

	conflict_all_t worst_all = conflict_all_t::only_one;
	std::vector<std::vector<conflict_this_t>> per_slot_this;

	if (entry.rec_type == "CELL")
	{
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
			for (size_t j = 0; j < parsed[i].size(); ++j)
			{
				if (parsed[i][j].type != "FRMR")
					continue;

				if (ver_refs[i].empty())
					ver_header_end[i] = j;

				uint32_t obj_idx = 0;
				if (parsed[i][j].size >= 4)
					std::memcpy(&obj_idx, parsed[i][j].data, 4);

				size_t end = parsed[i].size();
				for (size_t k = j + 1; k < parsed[i].size(); ++k)
				{
					if (parsed[i][k].type == "FRMR")
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
		}

		using slot_key_t = std::pair<std::string, int>;
		std::vector<slot_key_t> unified_slots;

		for (size_t i = 0; i < ver_count; ++i)
		{
			std::map<std::string, int> occ_count;
			for (size_t j = 0; j < ver_header_end[i]; ++j)
			{
				const auto & sv = parsed[i][j];
				int occ = occ_count[sv.type]++;
				slot_key_t key = { sv.type, occ };
				bool exists = false;
				for (const auto & s : unified_slots)
				{
					if (s == key)
					{
						exists = true;
						break;
					}
				}

				if (!exists)
					unified_slots.push_back(key);
			}
		}

		for (const auto & obj_idx : all_object_indices)
		{
			std::map<std::string, int> max_occ;

			for (size_t i = 0; i < ver_count; ++i)
			{
				for (const auto & ref : ver_refs[i])
				{
					if (ref.object_index != obj_idx)
						continue;

					std::map<std::string, int> occ_count;
					for (size_t j = ref.start_idx; j < ref.end_idx; ++j)
					{
						int occ = ++occ_count[parsed[i][j].type];
						if (occ > max_occ[parsed[i][j].type])
							max_occ[parsed[i][j].type] = occ;
					}

					break;
				}
			}

			for (const auto & [type, count] : max_occ)
			{
				for (int occ = 0; occ < count; ++occ)
					unified_slots.push_back({ type, occ });
			}
		}

		size_t header_slot_count = 0;
		{
			std::set<slot_key_t> seen;
			for (size_t i = 0; i < ver_count; ++i)
			{
				std::map<std::string, int> occ_count;
				for (size_t j = 0; j < ver_header_end[i]; ++j)
				{
					const auto & sv = parsed[i][j];
					int occ = occ_count[sv.type]++;
					seen.insert({ sv.type, occ });
				}
			}
			header_slot_count = seen.size();
		}

		std::vector<std::vector<std::string>> slot_values_all;
		for (const auto & slot : unified_slots)
			slot_values_all.push_back(std::vector<std::string>(ver_count));

		for (size_t i = 0; i < ver_count; ++i)
		{
			std::map<std::string, int> occ_count;
			for (size_t j = 0; j < ver_header_end[i]; ++j)
			{
				const auto & sv = parsed[i][j];
				int occ = occ_count[sv.type]++;
				slot_key_t key = { sv.type, occ };

				for (size_t si = 0; si < unified_slots.size(); ++si)
				{
					if (unified_slots[si] == key)
					{
						slot_values_all[si][i] = std::string(sv.data, sv.size);
						break;
					}
				}
			}
		}

		size_t slot_pos = header_slot_count;
		for (const auto & obj_idx : all_object_indices)
		{
			std::map<std::string, int> max_occ;
			for (size_t i = 0; i < ver_count; ++i)
			{
				for (const auto & ref : ver_refs[i])
				{
					if (ref.object_index != obj_idx)
						continue;

					std::map<std::string, int> occ_count;
					for (size_t j = ref.start_idx; j < ref.end_idx; ++j)
					{
						int occ = ++occ_count[parsed[i][j].type];
						if (occ > max_occ[parsed[i][j].type])
							max_occ[parsed[i][j].type] = occ;
					}

					break;
				}
			}

			for (size_t i = 0; i < ver_count; ++i)
			{
				for (const auto & ref : ver_refs[i])
				{
					if (ref.object_index != obj_idx)
						continue;

					std::map<std::string, int> occ_count;
					for (size_t j = ref.start_idx; j < ref.end_idx; ++j)
					{
						const auto & sv = parsed[i][j];
						int occ = occ_count[sv.type]++;

						size_t target_slot = slot_pos;
						for (const auto & [type, count] : max_occ)
						{
							if (type == sv.type)
							{
								slot_values_all[target_slot + occ][i] = std::string(sv.data, sv.size);
								break;
							}

							target_slot += count;
						}
					}

					break;
				}
			}

			for (const auto & [type, count] : max_occ)
				slot_pos += count;
		}

		for (size_t si = 0; si < unified_slots.size(); ++si)
		{
			auto & slot_values = slot_values_all[si];

			for (size_t vi = 0; vi < ver_count; ++vi)
			{
				if (is_deleted[vi])
					slot_values[vi] = "\x01\x44\x45\x4C\x45";
			}

			size_t first_size = 0;
			for (size_t vi = 0; vi < ver_count; ++vi)
			{
				if (!slot_values[vi].empty() && slot_values[vi][0] != '\x01')
				{
					first_size = slot_values[vi].size();
					break;
				}
			}

			const auto * schema = find_schema(entry.rec_type, unified_slots[si].first, first_size);
			if (schema && schema->field_count > 0 && first_size > 0)
			{
				for (size_t fi = 0; fi < schema->field_count; ++fi)
				{
					const auto & fdef = schema->fields[fi];
					std::vector<std::string> field_values(ver_count);

					for (size_t vi = 0; vi < ver_count; ++vi)
					{
						if (is_deleted[vi])
						{
							field_values[vi] = "\x01\x44\x45\x4C\x45";
							continue;
						}

						if (slot_values[vi].empty())
							continue;

						if (fdef.offset >= slot_values[vi].size())
							continue;

						size_t field_len = fdef.size;
						if (fdef.type == field_type_t::string_var)
							field_len = slot_values[vi].size() - fdef.offset;
						else
							field_len = std::min(fdef.size, slot_values[vi].size() - fdef.offset);

						field_values[vi] = slot_values[vi].substr(fdef.offset, field_len);
					}

					const auto field_level = compute_conflict_all(field_values);
					if (field_level > worst_all)
						worst_all = field_level;

					per_slot_this.push_back(compute_conflict_this(field_values));
				}
			}
			else
			{
				const auto slot_level = compute_conflict_all(slot_values);
				if (slot_level > worst_all)
					worst_all = slot_level;

				per_slot_this.push_back(compute_conflict_this(slot_values));
			}
		}
	}
	else if (entry.rec_type == "LEVI" || entry.rec_type == "LEVC")
	{
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
			for (size_t j = 0; j + 1 < parsed[i].size(); ++j)
			{
				if (parsed[i][j].type != "INTV" || parsed[i][j].size != 2)
					continue;

				if (parsed[i][j + 1].type != "INAM")
					continue;

				std::string item_id(parsed[i][j + 1].data, parsed[i][j + 1].size);
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

		for (size_t i = 0; i < ver_count; ++i)
		{
			std::map<std::string, int> occ_count;
			for (const auto & sv : parsed[i])
			{
				if (sv.type == "INTV" && sv.size == 2)
					continue;

				if (sv.type == "INAM")
					continue;

				int occ = occ_count[sv.type]++;
				slot_key_t key = { sv.type, occ };
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot == key)
					{
						found = true;
						break;
					}
				}

				if (!found)
					unified_slots.push_back(key);
			}
		}

		int intv_occ_base = 0;
		int inam_occ_base = 0;
		for (const auto & slot : unified_slots)
		{
			if (slot.first == "INTV")
				intv_occ_base = std::max(intv_occ_base, slot.second + 1);

			if (slot.first == "INAM")
				inam_occ_base = std::max(inam_occ_base, slot.second + 1);
		}

		for (size_t idx = 0; idx < all_item_ids.size(); ++idx)
		{
			unified_slots.push_back({ "INTV", intv_occ_base + static_cast<int>(idx) });
			unified_slots.push_back({ "INAM", inam_occ_base + static_cast<int>(idx) });
		}

		for (const auto & slot : unified_slots)
		{
			std::vector<std::string> slot_values(ver_count);
			bool is_item_intv = (slot.first == "INTV" && slot.second >= intv_occ_base);
			bool is_item_inam = (slot.first == "INAM" && slot.second >= inam_occ_base);

			for (size_t vi = 0; vi < ver_count; ++vi)
			{
				if (is_item_intv)
				{
					size_t item_idx = static_cast<size_t>(slot.second - intv_occ_base);
					const auto & target_id = all_item_ids[item_idx];
					for (const auto & e : ver_entries[vi])
					{
						if (e.item_id != target_id)
							continue;

						const auto & sv = parsed[vi][e.intv_idx];
						slot_values[vi] = std::string(sv.data, sv.size);
						break;
					}
				}
				else if (is_item_inam)
				{
					size_t item_idx = static_cast<size_t>(slot.second - inam_occ_base);
					const auto & target_id = all_item_ids[item_idx];
					for (const auto & e : ver_entries[vi])
					{
						if (e.item_id != target_id)
							continue;

						const auto & sv = parsed[vi][e.inam_idx];
						slot_values[vi] = std::string(sv.data, sv.size);
						break;
					}
				}
				else
				{
					int target_occ = slot.second;
					int current_occ = 0;
					for (const auto & sv : parsed[vi])
					{
						if (sv.type == "INTV" && sv.size == 2)
							continue;

						if (sv.type == "INAM")
							continue;

						if (sv.type != slot.first)
							continue;

						if (current_occ == target_occ)
						{
							slot_values[vi] = std::string(sv.data, sv.size);
							break;
						}

						++current_occ;
					}
				}

				if (is_deleted[vi])
					slot_values[vi] = "\x01\x44\x45\x4C\x45";
			}

			const auto slot_level = compute_conflict_all(slot_values);
			if (slot_level > worst_all)
				worst_all = slot_level;

			per_slot_this.push_back(compute_conflict_this(slot_values));
		}
	}
	else if (entry.rec_type == "FACT")
	{
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
			for (size_t j = 0; j + 1 < parsed[i].size(); ++j)
			{
				if (parsed[i][j].type != "INTV" || parsed[i][j].size != 4)
					continue;

				if (parsed[i][j + 1].type != "ANAM")
					continue;

				std::string faction_name(parsed[i][j + 1].data, parsed[i][j + 1].size);
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

		using slot_key_t = std::pair<std::string, int>;
		std::vector<slot_key_t> unified_slots;

		for (size_t i = 0; i < ver_count; ++i)
		{
			std::map<std::string, int> occ_count;
			for (const auto & sv : parsed[i])
			{
				if (sv.type == "INTV" && sv.size == 4)
					continue;

				if (sv.type == "ANAM")
					continue;

				int occ = occ_count[sv.type]++;
				slot_key_t key = { sv.type, occ };
				bool exists = false;
				for (const auto & s : unified_slots)
				{
					if (s == key)
					{
						exists = true;
						break;
					}
				}

				if (!exists)
					unified_slots.push_back(key);
			}
		}

		int intv_occ_base = 0;
		int anam_occ_base = 0;
		for (const auto & slot : unified_slots)
		{
			if (slot.first == "INTV")
				intv_occ_base = std::max(intv_occ_base, slot.second + 1);

			if (slot.first == "ANAM")
				anam_occ_base = std::max(anam_occ_base, slot.second + 1);
		}

		for (size_t idx = 0; idx < all_faction_names.size(); ++idx)
		{
			unified_slots.push_back({ "INTV", intv_occ_base + static_cast<int>(idx) });
			unified_slots.push_back({ "ANAM", anam_occ_base + static_cast<int>(idx) });
		}

		for (const auto & slot : unified_slots)
		{
			std::vector<std::string> slot_values(ver_count);
			bool is_faction_intv = (slot.first == "INTV" && slot.second >= intv_occ_base);
			bool is_faction_anam = (slot.first == "ANAM" && slot.second >= anam_occ_base);

			for (size_t vi = 0; vi < ver_count; ++vi)
			{
				if (is_faction_intv)
				{
					size_t item_idx = static_cast<size_t>(slot.second - intv_occ_base);
					const auto & target_name = all_faction_names[item_idx];
					for (const auto & e : ver_entries[vi])
					{
						if (e.faction_name != target_name)
							continue;

						const auto & sv = parsed[vi][e.intv_idx];
						slot_values[vi] = std::string(sv.data, sv.size);
						break;
					}
				}
				else if (is_faction_anam)
				{
					size_t item_idx = static_cast<size_t>(slot.second - anam_occ_base);
					const auto & target_name = all_faction_names[item_idx];
					for (const auto & e : ver_entries[vi])
					{
						if (e.faction_name != target_name)
							continue;

						const auto & sv = parsed[vi][e.anam_idx];
						slot_values[vi] = std::string(sv.data, sv.size);
						break;
					}
				}
				else
				{
					int target_occ = slot.second;
					int current_occ = 0;
					for (const auto & sv : parsed[vi])
					{
						if (sv.type == "INTV" && sv.size == 4)
							continue;

						if (sv.type == "ANAM")
							continue;

						if (sv.type != slot.first)
							continue;

						if (current_occ == target_occ)
						{
							slot_values[vi] = std::string(sv.data, sv.size);
							break;
						}

						++current_occ;
					}
				}

				if (is_deleted[vi])
					slot_values[vi] = "\x01\x44\x45\x4C\x45";
			}

			const auto slot_level = compute_conflict_all(slot_values);
			if (slot_level > worst_all)
				worst_all = slot_level;

			per_slot_this.push_back(compute_conflict_this(slot_values));
		}
	}
	else if (entry.rec_type == "CONT" || entry.rec_type == "CREA" || entry.rec_type == "NPC_")
	{
		struct cont_entry_t
		{
			std::string item_id;
			size_t npco_idx;
		};

		std::vector<std::vector<cont_entry_t>> ver_entries(ver_count);
		std::vector<std::string> all_item_ids;

		for (size_t i = 0; i < ver_count; ++i)
		{
			for (size_t j = 0; j < parsed[i].size(); ++j)
			{
				if (parsed[i][j].type != "NPCO" || parsed[i][j].size != 36)
					continue;

				std::string item_id(parsed[i][j].data + 4, 32);
				while (!item_id.empty() && item_id.back() == '\0')
					item_id.pop_back();

				ver_entries[i].push_back({ item_id, j });

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

		using slot_key_t = std::pair<std::string, int>;
		std::vector<slot_key_t> unified_slots;

		for (size_t i = 0; i < ver_count; ++i)
		{
			std::map<std::string, int> occ_count;
			for (const auto & sv : parsed[i])
			{
				if (sv.type == "NPCO" && sv.size == 36)
					continue;

				int occ = occ_count[sv.type]++;
				slot_key_t key = { sv.type, occ };
				bool exists = false;
				for (const auto & s : unified_slots)
				{
					if (s == key)
					{
						exists = true;
						break;
					}
				}

				if (!exists)
					unified_slots.push_back(key);
			}
		}

		int npco_occ_base = 0;
		for (const auto & slot : unified_slots)
		{
			if (slot.first == "NPCO")
				npco_occ_base = std::max(npco_occ_base, slot.second + 1);
		}

		for (size_t idx = 0; idx < all_item_ids.size(); ++idx)
			unified_slots.push_back({ "NPCO", npco_occ_base + static_cast<int>(idx) });

		for (const auto & slot : unified_slots)
		{
			std::vector<std::string> slot_values(ver_count);
			bool is_item_npco = (slot.first == "NPCO" && slot.second >= npco_occ_base);

			for (size_t vi = 0; vi < ver_count; ++vi)
			{
				if (is_item_npco)
				{
					size_t item_idx = static_cast<size_t>(slot.second - npco_occ_base);
					const auto & target_id = all_item_ids[item_idx];
					for (const auto & e : ver_entries[vi])
					{
						if (e.item_id != target_id)
							continue;

						const auto & sv = parsed[vi][e.npco_idx];
						slot_values[vi] = std::string(sv.data, sv.size);
						break;
					}
				}
				else
				{
					int target_occ = slot.second;
					int current_occ = 0;
					for (const auto & sv : parsed[vi])
					{
						if (sv.type == "NPCO" && sv.size == 36)
							continue;

						if (sv.type != slot.first)
							continue;

						if (current_occ == target_occ)
						{
							slot_values[vi] = std::string(sv.data, sv.size);
							break;
						}

						++current_occ;
					}
				}

				if (is_deleted[vi])
					slot_values[vi] = "\x01\x44\x45\x4C\x45";
			}

			size_t first_size = 0;
			for (size_t vi = 0; vi < ver_count; ++vi)
			{
				if (!slot_values[vi].empty() && slot_values[vi][0] != '\x01')
				{
					first_size = slot_values[vi].size();
					break;
				}
			}

			const auto * schema = find_schema(entry.rec_type, slot.first, first_size);
			if (schema && schema->field_count > 0 && first_size > 0)
			{
				for (size_t fi = 0; fi < schema->field_count; ++fi)
				{
					const auto & fdef = schema->fields[fi];
					std::vector<std::string> field_values(ver_count);

					for (size_t vi = 0; vi < ver_count; ++vi)
					{
						if (is_deleted[vi])
						{
							field_values[vi] = "\x01\x44\x45\x4C\x45";
							continue;
						}

						if (slot_values[vi].empty())
							continue;

						if (fdef.offset >= slot_values[vi].size())
							continue;

						size_t field_len = fdef.size;
						if (fdef.type == field_type_t::string_var)
							field_len = slot_values[vi].size() - fdef.offset;
						else
							field_len = std::min(fdef.size, slot_values[vi].size() - fdef.offset);

						field_values[vi] = slot_values[vi].substr(fdef.offset, field_len);
					}

					const auto field_level = compute_conflict_all(field_values);
					if (field_level > worst_all)
						worst_all = field_level;

					per_slot_this.push_back(compute_conflict_this(field_values));
				}
			}
			else
			{
				const auto slot_level = compute_conflict_all(slot_values);
				if (slot_level > worst_all)
					worst_all = slot_level;

				per_slot_this.push_back(compute_conflict_this(slot_values));
			}
		}
	}
	else
	{
		std::set<slot_key_t> seen;
		for (size_t i = 0; i < ver_count; ++i)
		{
			std::map<std::string, int> occ_count;
			for (const auto & sv : parsed[i])
			{
				int occ = occ_count[sv.type]++;
				slot_key_t key = { sv.type, occ };
				if (seen.insert(key).second)
					unified_slots.push_back(key);
			}
		}

		for (const auto & slot : unified_slots)
		{
			std::vector<std::string> slot_values(ver_count);
			size_t first_size = 0;

			for (size_t vi = 0; vi < ver_count; ++vi)
			{
				int target_occ = slot.second;
				int current_occ = 0;

				for (const auto & sv : parsed[vi])
				{
					if (sv.type != slot.first)
						continue;

					if (current_occ == target_occ)
					{
						slot_values[vi] = std::string(sv.data, sv.size);
						if (first_size == 0)
							first_size = sv.size;

						break;
					}

					++current_occ;
				}

				if (is_deleted[vi])
					slot_values[vi] = "\x01\x44\x45\x4C\x45";
			}

			const auto * schema = find_schema(entry.rec_type, slot.first, first_size);
			if (schema && schema->field_count > 0 && first_size > 0)
			{
				for (size_t fi = 0; fi < schema->field_count; ++fi)
				{
					const auto & fdef = schema->fields[fi];
					std::vector<std::string> field_values(ver_count);

					for (size_t vi = 0; vi < ver_count; ++vi)
					{
						if (is_deleted[vi])
						{
							field_values[vi] = "\x01\x44\x45\x4C\x45";
							continue;
						}

						if (slot_values[vi].empty())
							continue;

						if (fdef.offset >= slot_values[vi].size())
							continue;

						size_t field_len = fdef.size;
						if (fdef.type == field_type_t::string_var)
							field_len = slot_values[vi].size() - fdef.offset;
						else
							field_len = std::min(fdef.size, slot_values[vi].size() - fdef.offset);

						field_values[vi] = slot_values[vi].substr(fdef.offset, field_len);
					}

					const auto field_level = compute_conflict_all(field_values);
					if (field_level > worst_all)
						worst_all = field_level;

					per_slot_this.push_back(compute_conflict_this(field_values));
				}
			}
			else
			{
				const auto slot_level = compute_conflict_all(slot_values);
				if (slot_level > worst_all)
					worst_all = slot_level;

				per_slot_this.push_back(compute_conflict_this(slot_values));
			}
		}
	}

	entry.conflict_all = worst_all;

	if (worst_all <= conflict_all_t::only_one)
		return;

	std::vector<conflict_this_t> worst_this(ver_count, conflict_this_t::unknown);
	worst_this[0] = conflict_this_t::master;

	for (const auto & slot_ct : per_slot_this)
	{
		for (size_t i = 1; i < ver_count; ++i)
		{
			if (slot_ct[i] > worst_this[i])
				worst_this[i] = slot_ct[i];
		}
	}

	for (size_t i = 1; i < ver_count; ++i)
	{
		if (is_deleted[i])
		{
			entry.versions[i].status = conflict_this_t::deleted;
			continue;
		}

		entry.versions[i].status = worst_this[i];
	}
}

size_t plugin_scan_t::plugin_count() const
{
	return plugins_.size();
}

const std::string & plugin_scan_t::plugin_path(int idx) const
{
	return plugins_[idx]->path;
}

std::string plugin_scan_t::plugin_filename(int idx) const
{
	return std::filesystem::path(plugins_[idx]->path).filename().string();
}

const esm_reader_t & plugin_scan_t::plugin(int idx) const
{
	return plugins_[idx]->esm;
}

const plugin_index_t & plugin_scan_t::index(int idx) const
{
	return plugins_[idx]->index;
}

bool plugin_scan_t::is_merge_plugin(int idx) const
{
	return idx == merge_plugin_idx_;
}

const std::vector<conflict_entry_t> & plugin_scan_t::entries() const
{
	return entries_;
}

const conflict_entry_t * plugin_scan_t::find(const std::string & type, const std::string & id) const
{
	std::string key = type + std::string(1, '\0') + id;
	auto it = entry_lookup_.find(key);
	if (it == entry_lookup_.end())
		return nullptr;

	return &entries_[it->second];
}

std::vector<std::string> plugin_scan_t::all_types() const
{
	std::set<std::string> unique;
	for (const auto & entry : entries_)
		unique.insert(entry.rec_type);

	return std::vector<std::string>(unique.begin(), unique.end());
}

void plugin_scan_t::copy_record_to_merge(int source_plugin, size_t record_index)
{
	if (merge_plugin_idx_ < 0)
		return;

	plugins_[source_plugin]->esm.select_record(record_index);
	const auto & rec = plugins_[source_plugin]->esm.get_record();

	const auto & plugin_entries = plugins_[source_plugin]->index.entries();
	if (record_index >= plugin_entries.size())
		return;

	const auto & indexed = plugin_entries[record_index];

	merge_record_t mr;
	mr.rec_type = indexed.rec_type;
	mr.record_id = indexed.record_id;
	mr.content = rec.content;

	for (auto & existing : merge_records_)
	{
		if (existing.rec_type == mr.rec_type && existing.record_id == mr.record_id)
		{
			existing.content = mr.content;
			return;
		}
	}

	merge_records_.push_back(std::move(mr));
}

void plugin_scan_t::remove_from_merge(const std::string & type, const std::string & id)
{
	auto it = std::remove_if(
	    merge_records_.begin(),
	    merge_records_.end(),
	    [&](const merge_record_t & mr) { return mr.rec_type == type && mr.record_id == id; });
	merge_records_.erase(it, merge_records_.end());
}

bool plugin_scan_t::save_merge(
    const std::string & output_path,
    const std::string & author,
    const std::string & description)
{
	if (merge_plugin_idx_ < 0)
		return false;

	std::string header_content = build_tes3_header(author, description);

	std::vector<tools_t::record_t> records;
	records.push_back({ "TES3", header_content, header_content.size(), false });

	for (const auto & mr : merge_records_)
		records.push_back({ mr.rec_type, mr.content, mr.content.size(), false });

	tools_t::write_file(records, output_path);
	return true;
}

bool plugin_scan_t::has_merge() const
{
	return merge_plugin_idx_ >= 0;
}

size_t plugin_scan_t::merge_record_count() const
{
	return merge_records_.size();
}

const std::string & plugin_scan_t::merge_record_content(size_t index) const
{
	return merge_records_[index].content;
}

void plugin_scan_t::copy_record_to_merge_raw(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	if (merge_plugin_idx_ < 0)
		return;

	for (auto & existing : merge_records_)
	{
		if (existing.rec_type == rec_type && existing.record_id == record_id)
		{
			existing.content = content;
			return;
		}
	}

	merge_record_t mr;
	mr.rec_type = rec_type;
	mr.record_id = record_id;
	mr.content = content;
	merge_records_.push_back(std::move(mr));
}

void plugin_scan_t::merge_leveled_list(const conflict_entry_t & entry)
{
	if (entry.versions.size() < 2)
		return;

	struct list_item_t
	{
		std::string id;
		uint16_t level;
	};

	auto extract_items = [&](const std::string & content) -> std::vector<list_item_t>
	{
		std::vector<list_item_t> items;
		sub_record_iter_t iter(content);
		sub_record_view_t sub;
		std::string current_id;

		while (iter.next(sub))
		{
			if (sub.type == "INAM" || sub.type == "CNAM")
			{
				current_id = std::string(sub.data, sub.size);
				current_id = tools_t::erase_null_chars(current_id);
				continue;
			}

			if (sub.type == "INTV" && !current_id.empty())
			{
				uint16_t level = 0;
				if (sub.size >= 2)
					std::memcpy(&level, sub.data, 2);

				items.push_back({ current_id, level });
				current_id.clear();
			}
		}

		return items;
	};

	auto get_content = [&](const record_version_t & ver) -> std::string
	{
		if (ver.plugin_idx == merge_plugin_idx_)
			return merge_records_[ver.record_index].content;

		plugins_[ver.plugin_idx]->esm.select_record(ver.record_index);
		return plugins_[ver.plugin_idx]->esm.get_record().content;
	};

	std::string master_content = get_content(entry.versions[0]);
	auto master_items = extract_items(master_content);

	std::set<std::string> master_keys;
	for (const auto & item : master_items)
		master_keys.insert(item.id + "\x00" + std::to_string(item.level));

	std::vector<list_item_t> merged = master_items;
	std::set<std::string> merged_keys = master_keys;

	for (size_t vi = 1; vi < entry.versions.size(); ++vi)
	{
		if (entry.versions[vi].plugin_idx == merge_plugin_idx_)
			continue;

		std::string ver_content = get_content(entry.versions[vi]);
		auto ver_items = extract_items(ver_content);

		for (const auto & item : ver_items)
		{
			std::string key = item.id + "\x00" + std::to_string(item.level);
			if (merged_keys.count(key))
				continue;

			merged.push_back(item);
			merged_keys.insert(key);
		}
	}

	std::string header_part;
	{
		sub_record_iter_t iter(master_content);
		sub_record_view_t sub;
		while (iter.next(sub))
		{
			if (sub.type == "INAM" || sub.type == "CNAM" || sub.type == "INTV")
				break;

			header_part += sub.type;
			header_part += tools_t::convert_uint_to_string_byte_array(sub.size);
			header_part += std::string(sub.data, sub.size);
		}
	}

	std::string indx_sub = "INDX";
	uint32_t item_count = static_cast<uint32_t>(merged.size());
	indx_sub += tools_t::convert_uint_to_string_byte_array(4);
	indx_sub += std::string(reinterpret_cast<const char *>(&item_count), 4);

	std::string items_part;
	std::string item_sub_type = (entry.rec_type == "LEVI") ? "INAM" : "CNAM";
	for (const auto & item : merged)
	{
		std::string id_data = item.id;
		id_data.push_back('\0');

		items_part += item_sub_type;
		items_part += tools_t::convert_uint_to_string_byte_array(id_data.size());
		items_part += id_data;

		items_part += "INTV";
		items_part += tools_t::convert_uint_to_string_byte_array(2);
		items_part += std::string(reinterpret_cast<const char *>(&item.level), 2);
	}

	std::string body = header_part + indx_sub + items_part;

	std::string record;
	record += entry.rec_type;
	record += tools_t::convert_uint_to_string_byte_array(body.size());
	record += std::string(8, '\0');
	record += body;

	copy_record_to_merge_raw(entry.rec_type, entry.record_id, record);
}

void plugin_scan_t::merge_dialogue(const conflict_entry_t & entry)
{
	if (entry.rec_type != "DIAL")
		return;

	auto get_content = [&](const record_version_t & ver) -> std::string
	{
		if (ver.plugin_idx == merge_plugin_idx_)
			return merge_records_[ver.record_index].content;

		plugins_[ver.plugin_idx]->esm.select_record(ver.record_index);
		return plugins_[ver.plugin_idx]->esm.get_record().content;
	};

	std::string winning_dial = get_content(entry.versions.back());
	copy_record_to_merge_raw("DIAL", entry.record_id, winning_dial);

	std::vector<std::string> merged_info_ids;
	std::map<std::string, std::string> info_contents;

	for (size_t vi = 0; vi < entry.versions.size(); ++vi)
	{
		const auto & ver = entry.versions[vi];
		if (ver.plugin_idx == merge_plugin_idx_)
			continue;

		int pi = ver.plugin_idx;
		size_t dial_rec_idx = ver.record_index;

		const auto & plugin_entries = plugins_[pi]->index.entries();
		for (size_t ei = dial_rec_idx + 1; ei < plugin_entries.size(); ++ei)
		{
			if (plugin_entries[ei].rec_type != "INFO")
				break;

			if (plugin_entries[ei].dial_name != entry.record_id)
				break;

			const auto & info_id = plugin_entries[ei].record_id;

			plugins_[pi]->esm.select_record(plugin_entries[ei].record_index);
			std::string content = plugins_[pi]->esm.get_record().content;

			if (info_contents.find(info_id) == info_contents.end())
				merged_info_ids.push_back(info_id);

			info_contents[info_id] = content;
		}
	}

	for (const auto & info_id : merged_info_ids)
	{
		const auto & content = info_contents[info_id];
		copy_record_to_merge_raw("INFO", info_id, content);
	}
}

size_t plugin_scan_t::itm_count(int plugin_idx) const
{
	size_t count = 0;
	for (const auto & entry : entries_)
	{
		for (const auto & ver : entry.versions)
		{
			if (ver.plugin_idx == plugin_idx && ver.status == conflict_this_t::identical_to_master)
			{
				++count;
				break;
			}
		}
	}
	return count;
}

std::vector<const conflict_entry_t *> plugin_scan_t::itm_entries(int plugin_idx) const
{
	std::vector<const conflict_entry_t *> result;
	for (const auto & entry : entries_)
	{
		for (const auto & ver : entry.versions)
		{
			if (ver.plugin_idx == plugin_idx && ver.status == conflict_this_t::identical_to_master)
			{
				result.push_back(&entry);
				break;
			}
		}
	}
	return result;
}

std::string plugin_scan_t::build_tes3_header(const std::string & author, const std::string & description)
{
	std::string hedr_data(300, '\0');

	float version = 1.3f;
	std::memcpy(&hedr_data[0], &version, 4);

	uint32_t flags = 0;
	std::memcpy(&hedr_data[4], &flags, 4);

	for (size_t i = 0; i < author.size() && i < 32; ++i)
		hedr_data[8 + i] = author[i];

	for (size_t i = 0; i < description.size() && i < 256; ++i)
		hedr_data[40 + i] = description[i];

	uint32_t num_records = static_cast<uint32_t>(merge_records_.size());
	std::memcpy(&hedr_data[296], &num_records, 4);

	std::string body;
	body += "HEDR";
	body += tools_t::convert_uint_to_string_byte_array(300);
	body += hedr_data;

	for (int i = 0; i < static_cast<int>(plugins_.size()); ++i)
	{
		if (i == merge_plugin_idx_)
			continue;

		std::string filename = plugin_filename(i);
		filename.push_back('\0');

		body += "MAST";
		body += tools_t::convert_uint_to_string_byte_array(filename.size());
		body += filename;

		uint64_t file_size = 0;
		try
		{
			file_size = std::filesystem::file_size(plugins_[i]->path);
		}
		catch (...)
		{}

		std::string size_data(8, '\0');
		std::memcpy(&size_data[0], &file_size, 8);

		body += "DATA";
		body += tools_t::convert_uint_to_string_byte_array(8);
		body += size_data;
	}

	std::string header;
	header += "TES3";
	header += tools_t::convert_uint_to_string_byte_array(body.size());

	std::string unknown(4, '\0');
	header += unknown;

	std::string rec_flags(4, '\0');
	header += rec_flags;

	header += body;

	return header;
}
