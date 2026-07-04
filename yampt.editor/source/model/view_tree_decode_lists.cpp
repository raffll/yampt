#include "view_tree_model.hpp"
#include <decoder/conflict_slots.hpp>
#include <decoder/view_tree_format.hpp>
#include <scanner/conflict_compute.hpp>
#include <algorithm>
#include <cstring>

static void build_unified_slots_from_alignment(
    const slot_result_t & slot_result,
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots)
{
	for (const auto & aligned_slot : slot_result.aligned)
		unified_slots.push_back({ aligned_slot.key.type, aligned_slot.key.occurrence });
}

static void build_col_indices_from_alignment(
    const slot_result_t & slot_result,
    size_t col_count,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices)
{
	for (const auto & aligned_slot : slot_result.aligned)
	{
		const auto ver_size = aligned_slot.indices.size();
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col < ver_size)
				col_indices[col][aligned_slot.key.type].push_back(aligned_slot.indices[col]);
			else
				col_indices[col][aligned_slot.key.type].push_back(SIZE_MAX);
		}
	}
}

template<typename Predicate>
static void build_non_excluded_slots(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    Predicate is_excluded,
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots)
{
	std::unordered_map<std::string, int> type_count;
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (const auto & sv : all_subs[col])
		{
			if (is_excluded(sv))
				continue;

			int occur = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : unified_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occur)
				{
					found = true;
					break;
				}
			}

			if (!found)
				unified_slots.push_back({ sv.type, occur });
		}
		type_count.clear();
	}
}

static void append_pair_slots(
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots,
    const std::vector<std::string> & entry_keys,
    const std::string & type_first,
    const std::string & type_second)
{
	for (size_t i = 0; i < entry_keys.size(); ++i)
	{
		int occ_first = -1;
		int occ_second = -1;
		for (const auto & slot : unified_slots)
		{
			if (slot.type == type_first)
				occ_first = std::max(occ_first, slot.occurrence);

			if (slot.type == type_second)
				occ_second = std::max(occ_second, slot.occurrence);
		}
		unified_slots.push_back({ type_first, occ_first + 1 });
		unified_slots.push_back({ type_second, occ_second + 1 });
	}
}

static void append_single_slots(
    std::vector<view_tree_model_t::sub_slot_t> & unified_slots,
    const std::vector<std::string> & entry_keys,
    const std::string & slot_type)
{
	for (size_t i = 0; i < entry_keys.size(); ++i)
	{
		int max_occ = -1;
		for (const auto & slot : unified_slots)
		{
			if (slot.type == slot_type)
				max_occ = std::max(max_occ, slot.occurrence);
		}
		unified_slots.push_back({ slot_type, max_occ + 1 });
	}
}

template<typename Predicate>
static void build_non_excluded_indices(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    Predicate is_excluded,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (size_t i = 0; i < all_subs[col].size(); ++i)
		{
			if (is_excluded(all_subs[col][i]))
				continue;

			col_indices[col][all_subs[col][i].type].push_back(i);
		}
	}
}

struct paired_entry_t
{
	std::string entry_id;
	size_t first_idx;
	size_t second_idx;
};

struct single_entry_t
{
	std::string entry_id;
	size_t sub_idx;
};

static void match_paired_entries(
    const std::vector<std::vector<paired_entry_t>> & col_entries,
    const std::vector<std::string> & all_ids,
    const std::string & type_first,
    const std::string & type_second,
    size_t col_count,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= col_entries.size())
			continue;

		for (const auto & target_id : all_ids)
		{
			bool matched = false;
			for (const auto & entry : col_entries[col])
			{
				if (entry.entry_id != target_id)
					continue;

				col_type_indices[col][type_first].push_back(entry.first_idx);
				col_type_indices[col][type_second].push_back(entry.second_idx);
				matched = true;
				break;
			}

			if (!matched)
			{
				col_type_indices[col][type_first].push_back(SIZE_MAX);
				col_type_indices[col][type_second].push_back(SIZE_MAX);
			}
		}
	}
}

static void match_single_entries(
    const std::vector<std::vector<single_entry_t>> & col_entries,
    const std::vector<std::string> & all_ids,
    const std::string & slot_type,
    size_t col_count,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= col_entries.size())
			continue;

		for (const auto & target_id : all_ids)
		{
			bool matched = false;
			for (const auto & entry : col_entries[col])
			{
				if (entry.entry_id != target_id)
					continue;

				col_type_indices[col][slot_type].push_back(entry.sub_idx);
				matched = true;
				break;
			}

			if (!matched)
				col_type_indices[col][slot_type].push_back(SIZE_MAX);
		}
	}
}

void view_tree_model_t::set_record_leveled(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
		build_col_indices_from_alignment(*entry.slot_result, col_count, col_type_indices);
	}
	else
	{
		collect_leveled_entries(context, build_ctx);
	}

	emit_leveled_rows(context, build_ctx);
}

void view_tree_model_t::set_record_faction(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
		build_col_indices_from_alignment(*entry.slot_result, col_count, col_type_indices);
	}
	else
	{
		collect_faction_entries(context, build_ctx);
	}

	emit_slot_rows(context, build_ctx);
}

void view_tree_model_t::set_record_container(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
		build_col_indices_from_alignment(*entry.slot_result, col_count, col_type_indices);
	}
	else
	{
		collect_container_entries(context, build_ctx);
	}

	emit_slot_rows(context, build_ctx);
}

void view_tree_model_t::set_record_armor(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<sub_slot_t> unified_slots;
	if (entry.slot_result)
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
	else
	{
		for (const auto & subs : all_subs)
		{
			std::unordered_map<std::string, int> type_count;
			for (const auto & sv : subs)
			{
				int occur = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occur)
					{
						found = true;
						break;
					}
				}

				if (!found)
					unified_slots.push_back({ sv.type, occur });
			}
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	if (entry.slot_result)
	{
		build_col_indices_from_alignment(*entry.slot_result, col_count, col_type_indices);
	}
	else
	{
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_subs.size())
				continue;

			for (size_t i = 0; i < all_subs[col].size(); ++i)
				col_type_indices[col][all_subs[col][i].type].push_back(i);
		}
	}

	int part_index = 0;

	for (size_t i = 0; i < unified_slots.size(); ++i)
	{
		bool is_group = (unified_slots[i].type == "INDX") &&
		                (i + 1 < unified_slots.size()) &&
		                (unified_slots[i + 1].type == "BNAM" || unified_slots[i + 1].type == "CNAM");

		if (!is_group)
		{
			m_rows.push_back(build_slot_row(col_count, all_subs, col_type_indices, unified_slots[i]));
			continue;
		}

		auto indx_row = build_slot_row(col_count, all_subs, col_type_indices, unified_slots[i]);

		sub_record_row_t group_row;
		group_row.type = "INDX";
		group_row.size = 0;
		group_row.label = "Body Part #" + std::to_string(part_index);
		group_row.values = indx_row.values;
		group_row.all_identical = indx_row.all_identical;

		field_row_t indx_field;
		indx_field.name = "INDX - Armor Index";
		if (!indx_row.children.empty())
			indx_field.values = indx_row.children[0].values;
		else
			indx_field.values = indx_row.values;

		indx_field.all_identical = indx_row.all_identical;
		indx_field.row_conflict_all = indx_row.row_conflict_all;
		indx_field.cell_conflict_this = indx_row.cell_conflict_this;
		group_row.children.push_back(std::move(indx_field));

		size_t next = i + 1;
		while (next < unified_slots.size() &&
		       (unified_slots[next].type == "BNAM" || unified_slots[next].type == "CNAM"))
		{
			auto part_row = build_slot_row(col_count, all_subs, col_type_indices, unified_slots[next]);

			field_row_t part_field;
			part_field.name =
			    (unified_slots[next].type == "BNAM") ? "BNAM - Male Part Name" : "CNAM - Female Part Name";
			part_field.values = part_row.values;
			part_field.all_identical = part_row.all_identical;
			part_field.row_conflict_all = part_row.row_conflict_all;
			part_field.cell_conflict_this = part_row.cell_conflict_this;
			group_row.children.push_back(std::move(part_field));
			++next;
		}

		group_row.row_conflict_all = conflict_all_t::only_one;
		group_row.cell_conflict_this.resize(col_count, conflict_this_t::unknown);
		for (const auto & child : group_row.children)
		{
			if (child.row_conflict_all > group_row.row_conflict_all)
				group_row.row_conflict_all = child.row_conflict_all;

			for (size_t col = 0; col < col_count && col < child.cell_conflict_this.size(); ++col)
			{
				if (child.cell_conflict_this[col] > group_row.cell_conflict_this[col])
					group_row.cell_conflict_this[col] = child.cell_conflict_this[col];
			}
		}

		m_rows.push_back(std::move(group_row));
		++part_index;
		i = next - 1;
	}
}

void view_tree_model_t::set_record_generic(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<sub_slot_t> unified_slots;

	if (entry.slot_result)
	{
		build_unified_slots_from_alignment(*entry.slot_result, unified_slots);
	}
	else
	{
		for (const auto & subs : all_subs)
		{
			std::unordered_map<std::string, int> type_count;
			for (const auto & sv : subs)
			{
				int occur = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occur)
					{
						found = true;
						break;
					}
				}

				if (!found)
					unified_slots.push_back({ sv.type, occur });
			}
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (size_t i = 0; i < all_subs[col].size(); ++i)
			col_type_indices[col][all_subs[col][i].type].push_back(i);
	}

	for (const auto & slot : unified_slots)
		m_rows.push_back(build_slot_row(col_count, all_subs, col_type_indices, slot));
}

void view_tree_model_t::collect_leveled_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	static constexpr size_t leveled_intv_size = 2;
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	const std::string id_type = (m_record_type == "LEVC") ? "CNAM" : "INAM";

	std::vector<std::vector<paired_entry_t>> col_entries(col_count);
	std::vector<std::string> all_item_ids;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i + 1 < subs.size(); ++i)
		{
			if (subs[i].type != id_type)
				continue;

			if (subs[i + 1].type != "INTV" || subs[i + 1].size != leveled_intv_size)
				continue;

			std::string item_id(subs[i].data, subs[i].size);
			if (!item_id.empty() && item_id.back() == '\0')
				item_id.pop_back();

			col_entries[col].push_back({ item_id, i, i + 1 });

			if (std::find(all_item_ids.begin(), all_item_ids.end(), item_id) == all_item_ids.end())
				all_item_ids.push_back(item_id);
		}
	}

	auto is_excluded = [&id_type](const sub_record_view_t & sv_rec)
	{ return (sv_rec.type == "INTV" && sv_rec.size == 2) || sv_rec.type == id_type; };

	build_non_excluded_slots(all_subs, col_count, is_excluded, build_ctx.unified_slots);
	append_pair_slots(build_ctx.unified_slots, all_item_ids, id_type, "INTV");
	build_non_excluded_indices(all_subs, col_count, is_excluded, build_ctx.col_type_indices);
	match_paired_entries(col_entries, all_item_ids, id_type, "INTV", col_count, build_ctx.col_type_indices);
}

void view_tree_model_t::collect_faction_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	static constexpr size_t faction_intv_size = 4;
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<std::vector<paired_entry_t>> col_entries(col_count);
	std::vector<std::string> all_faction_names;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i + 1 < subs.size(); ++i)
		{
			if (subs[i].type != "INTV" || subs[i].size != faction_intv_size || subs[i + 1].type != "ANAM")
				continue;

			std::string faction_name(subs[i + 1].data, subs[i + 1].size);
			if (!faction_name.empty() && faction_name.back() == '\0')
				faction_name.pop_back();

			col_entries[col].push_back({ faction_name, i, i + 1 });

			if (std::find(all_faction_names.begin(), all_faction_names.end(), faction_name) == all_faction_names.end())
				all_faction_names.push_back(faction_name);
		}
	}

	auto is_excluded = [](const sub_record_view_t & sv_rec)
	{ return (sv_rec.type == "INTV" && sv_rec.size == 4) || sv_rec.type == "ANAM"; };

	build_non_excluded_slots(all_subs, col_count, is_excluded, build_ctx.unified_slots);
	append_pair_slots(build_ctx.unified_slots, all_faction_names, "INTV", "ANAM");
	build_non_excluded_indices(all_subs, col_count, is_excluded, build_ctx.col_type_indices);
	match_paired_entries(col_entries, all_faction_names, "INTV", "ANAM", col_count, build_ctx.col_type_indices);
}

void view_tree_model_t::collect_container_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	static constexpr size_t npco_record_size = 36;
	static constexpr size_t npco_name_offset = 4;
	static constexpr size_t npco_name_length = 32;
	static constexpr size_t npcs_record_size = 32;

	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<std::vector<single_entry_t>> col_items(col_count);
	std::vector<std::vector<single_entry_t>> col_spells(col_count);
	std::vector<std::string> all_item_ids;
	std::vector<std::string> all_spell_ids;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i < subs.size(); ++i)
		{
			if (subs[i].type == "NPCO" && subs[i].size == npco_record_size)
			{
				std::string item_id(subs[i].data + npco_name_offset, npco_name_length);
				while (!item_id.empty() && item_id.back() == '\0')
					item_id.pop_back();

				col_items[col].push_back({ item_id, i });
				if (std::find(all_item_ids.begin(), all_item_ids.end(), item_id) == all_item_ids.end())
					all_item_ids.push_back(item_id);
			}
			else if (subs[i].type == "NPCS" && subs[i].size == npcs_record_size)
			{
				std::string spell_id(subs[i].data, npcs_record_size);
				while (!spell_id.empty() && spell_id.back() == '\0')
					spell_id.pop_back();

				col_spells[col].push_back({ spell_id, i });
				if (std::find(all_spell_ids.begin(), all_spell_ids.end(), spell_id) == all_spell_ids.end())
					all_spell_ids.push_back(spell_id);
			}
		}
	}

	auto is_excluded = [](const sub_record_view_t & sv_rec)
	{ return (sv_rec.type == "NPCO" && sv_rec.size == 36) || (sv_rec.type == "NPCS" && sv_rec.size == 32); };
	build_non_excluded_slots(all_subs, col_count, is_excluded, build_ctx.unified_slots);
	append_single_slots(build_ctx.unified_slots, all_item_ids, "NPCO");
	append_single_slots(build_ctx.unified_slots, all_spell_ids, "NPCS");
	build_non_excluded_indices(all_subs, col_count, is_excluded, build_ctx.col_type_indices);
	match_single_entries(col_items, all_item_ids, "NPCO", col_count, build_ctx.col_type_indices);
	match_single_entries(col_spells, all_spell_ids, "NPCS", col_count, build_ctx.col_type_indices);
}

void view_tree_model_t::emit_slot_rows(record_context_t & context, slot_build_context_t & build_ctx)
{
	for (const auto & slot : build_ctx.unified_slots)
		m_rows.push_back(build_slot_row(context.col_count, context.all_sub_records, build_ctx.col_type_indices, slot));
}

void view_tree_model_t::emit_leveled_rows(record_context_t & context, slot_build_context_t & build_ctx)
{
	const auto & unified = build_ctx.unified_slots;
	const auto col_count = context.col_count;
	const bool is_creature = (m_record_type == "LEVC");
	std::string id_type = is_creature ? "CNAM" : "INAM";
	int entry_index = 0;

	for (size_t i = 0; i < unified.size(); ++i)
	{
		bool is_pair = (unified[i].type == id_type) && (i + 1 < unified.size()) && (unified[i + 1].type == "INTV");

		if (!is_pair)
		{
			m_rows.push_back(
			    build_slot_row(col_count, context.all_sub_records, build_ctx.col_type_indices, unified[i]));
			continue;
		}

		auto id_row = build_slot_row(col_count, context.all_sub_records, build_ctx.col_type_indices, unified[i]);
		auto level_row = build_slot_row(col_count, context.all_sub_records, build_ctx.col_type_indices, unified[i + 1]);

		sub_record_row_t group_row;
		group_row.type = id_type;
		group_row.size = 0;
		group_row.values = id_row.values;
		group_row.all_identical = id_row.all_identical && level_row.all_identical;

		if (is_creature)
			group_row.label = "Leveled Creature #" + std::to_string(entry_index);
		else
			group_row.label = "Leveled Item #" + std::to_string(entry_index);

		field_row_t name_field;
		name_field.name = is_creature ? "CNAM - Creature Name" : "INAM - Item";
		name_field.values = id_row.values;
		name_field.all_identical = id_row.all_identical;
		name_field.row_conflict_all = id_row.row_conflict_all;
		name_field.cell_conflict_this = id_row.cell_conflict_this;

		field_row_t level_field;
		level_field.name = "INTV - PC Level";
		if (!level_row.children.empty())
			level_field.values = level_row.children[0].values;
		else
			level_field.values = level_row.values;

		level_field.all_identical = level_row.all_identical;
		level_field.row_conflict_all = level_row.row_conflict_all;
		level_field.cell_conflict_this = level_row.cell_conflict_this;

		group_row.children.push_back(std::move(name_field));
		group_row.children.push_back(std::move(level_field));

		group_row.row_conflict_all = (id_row.row_conflict_all > level_row.row_conflict_all)
		                                 ? id_row.row_conflict_all
		                                 : level_row.row_conflict_all;
		group_row.cell_conflict_this.resize(col_count);

		for (size_t col = 0; col < col_count; ++col)
		{
			conflict_this_t id_ct = conflict_this_t::unknown;
			conflict_this_t lv_ct = conflict_this_t::unknown;

			if (col < id_row.cell_conflict_this.size())
				id_ct = id_row.cell_conflict_this[col];

			if (col < level_row.cell_conflict_this.size())
				lv_ct = level_row.cell_conflict_this[col];

			group_row.cell_conflict_this[col] = (id_ct > lv_ct) ? id_ct : lv_ct;
		}

		m_rows.push_back(std::move(group_row));
		++entry_index;
		++i;
	}
}
