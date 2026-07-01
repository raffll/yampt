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

	emit_slot_rows(context, build_ctx);
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

	std::vector<std::vector<paired_entry_t>> col_entries(col_count);
	std::vector<std::string> all_item_ids;

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i + 1 < subs.size(); ++i)
		{
			if (subs[i].type != "INTV" || subs[i].size != leveled_intv_size || subs[i + 1].type != "INAM")
				continue;

			std::string item_id(subs[i + 1].data, subs[i + 1].size);
			if (!item_id.empty() && item_id.back() == '\0')
				item_id.pop_back();

			col_entries[col].push_back({ item_id, i, i + 1 });

			if (std::find(all_item_ids.begin(), all_item_ids.end(), item_id) == all_item_ids.end())
				all_item_ids.push_back(item_id);
		}
	}

	auto is_excluded = [](const sub_record_view_t & sv_rec)
	{ return (sv_rec.type == "INTV" && sv_rec.size == 2) || sv_rec.type == "INAM"; };

	build_non_excluded_slots(all_subs, col_count, is_excluded, build_ctx.unified_slots);
	append_pair_slots(build_ctx.unified_slots, all_item_ids, "INTV", "INAM");
	build_non_excluded_indices(all_subs, col_count, is_excluded, build_ctx.col_type_indices);
	match_paired_entries(col_entries, all_item_ids, "INTV", "INAM", col_count, build_ctx.col_type_indices);
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
