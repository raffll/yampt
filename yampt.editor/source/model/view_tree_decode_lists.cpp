#include "view_tree_model.hpp"
#include <decoder/conflict_slots.hpp>
#include <decoder/content_alignment.hpp>
#include <decoder/scvr_condition.hpp>
#include <decoder/view_tree_format.hpp>
#include <scanner/dial_info_align.hpp>
#include <scanner/record_conflict.hpp>
#include <algorithm>

namespace
{
	const sub_record_view_t * find_slot_view(
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    const std::unordered_map<std::string, std::vector<size_t>> & type_index,
	    const sub_slot_t & slot,
	    size_t column)
	{
		if (column >= all_subs.size())
			return nullptr;

		auto it_type = type_index.find(slot.type);
		if (it_type == type_index.end() || slot.occurrence < 0 ||
		    slot.occurrence >= static_cast<int>(it_type->second.size()))
			return nullptr;

		const size_t idx = it_type->second[slot.occurrence];
		if (idx == SIZE_MAX || idx >= all_subs[column].size())
			return nullptr;

		return &all_subs[column][idx];
	}
}

void view_tree_model_t::set_record_leveled(record_context_t & context, const conflict_entry_t & entry)
{
	(void)entry;
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	collect_leveled_entries(context, build_ctx);
	emit_leveled_rows(context, build_ctx);
}

void view_tree_model_t::set_record_faction(record_context_t & context, const conflict_entry_t & entry)
{
	(void)entry;
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	collect_faction_entries(context, build_ctx);
	emit_slot_rows(context, build_ctx);
}

void view_tree_model_t::set_record_container(record_context_t & context, const conflict_entry_t & entry)
{
	(void)entry;
	const auto col_count = context.col_count;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	slot_build_context_t build_ctx { unified_slots, col_type_indices };

	collect_container_entries(context, build_ctx);
	emit_slot_rows(context, build_ctx);
}

void view_tree_model_t::set_record_armor(record_context_t & context, const conflict_entry_t & entry)
{
	(void)entry;
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);

	alignment_rule_t rule;
	rule.anchor_type = "INDX";
	rule.anchor_size = 0;
	rule.trailing_types = { "BNAM", "CNAM" };
	rule.key_source = alignment_rule_t::key_from_t::anchor;

	alignment_context_t align_ctx { all_subs, col_count, unified_slots, col_type_indices };
	content_alignment_t::align(align_ctx, { rule });

	auto is_body_part_type = [](const std::string & slot_type)
	{ return slot_type == "INDX" || slot_type == "BNAM" || slot_type == "CNAM"; };

	std::stable_sort(
	    unified_slots.begin(),
	    unified_slots.end(),
	    [&](const sub_slot_t & left, const sub_slot_t & right)
	{ return !is_body_part_type(left.type) && is_body_part_type(right.type); });

	int part_index = 0;
	for (size_t i = 0; i < unified_slots.size(); ++i)
	{
		if (unified_slots[i].type != "INDX")
		{
			m_rows.push_back(build_slot_row(col_count, all_subs, col_type_indices, unified_slots[i]));
			continue;
		}

		auto indx_row = build_slot_row(col_count, all_subs, col_type_indices, unified_slots[i]);

		view_node_t group_row;
		group_row.type = "INDX";
		group_row.size = 0;
		group_row.label = "Body Part #" + std::to_string(part_index);
		group_row.values = indx_row.values;
		group_row.all_identical = indx_row.all_identical;

		view_node_t indx_field;
		indx_field.label = "INDX - Armor Index";
		indx_field.type = indx_row.type;
		indx_field.size = indx_row.size;
		indx_field.binary_ranges = indx_row.binary_ranges;
		indx_field.values = indx_row.children.empty() ? indx_row.values : indx_row.children[0].values;
		indx_field.all_identical = indx_row.all_identical;
		indx_field.row_conflict_all = indx_row.row_conflict_all;
		indx_field.cell_conflict_this = indx_row.cell_conflict_this;
		group_row.children.push_back(std::move(indx_field));

		size_t next = i + 1;
		while (next < unified_slots.size() &&
		       (unified_slots[next].type == "BNAM" || unified_slots[next].type == "CNAM"))
		{
			auto part_row = build_slot_row(col_count, all_subs, col_type_indices, unified_slots[next]);

			view_node_t part_field;
			part_field.label =
			    (unified_slots[next].type == "BNAM") ? "BNAM - Male Part Name" : "CNAM - Female Part Name";
			part_field.type = part_row.type;
			part_field.size = part_row.size;
			part_field.binary_ranges = part_row.binary_ranges;
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

		compute_group_ranges(group_row, col_count);

		m_rows.push_back(std::move(group_row));
		++part_index;
		i = next - 1;
	}
}

void view_tree_model_t::set_record_dial(
    plugin_scan_t & scan,
    record_context_t & context,
    const conflict_entry_t & entry)
{
	set_record_generic(context, entry);

	const auto col_count = context.col_count;
	const auto info_result = dial_info_align_t::build(scan, entry.record_id);

	if (info_result.entries.empty())
		return;

	view_node_t chain_group;
	chain_group.label = "Dialogue Responses";
	chain_group.is_info_chain = true;
	chain_group.values.resize(col_count);
	chain_group.cell_conflict_this.resize(col_count, conflict_this_t::unknown);
	chain_group.row_conflict_all = conflict_all_t::only_one;

	for (const auto & info_entry : info_result.entries)
	{
		view_node_t info_row;
		info_row.label = info_entry.inam;
		info_row.is_info_chain = true;
		info_row.values.resize(col_count);
		info_row.cell_conflict_this.resize(col_count, conflict_this_t::unknown);

		for (size_t col = 0; col < col_count; ++col)
		{
			const int plugin_idx = (col < m_column_plugin_indices.size()) ? m_column_plugin_indices[col] : -1;

			const bool present = plugin_idx >= 0 &&
			    plugin_idx < static_cast<int>(info_entry.present_in_plugin.size()) &&
			    info_entry.present_in_plugin[plugin_idx];

			if (!present)
			{
				info_row.values[col] = non_existent_value;
				continue;
			}

			info_row.values[col] = info_entry.text_in_plugin[plugin_idx];
		}

		bool all_same = true;
		for (size_t col = 1; col < col_count; ++col)
		{
			if (info_row.values[col] != info_row.values[0])
			{
				all_same = false;
				break;
			}
		}
		info_row.all_identical = all_same;

		info_row.row_conflict_all = record_conflict::compute_conflict_all_skip_empty(info_row.values);
		info_row.cell_conflict_this = record_conflict::compute_conflict_this_skip_empty(info_row.values);

		if (info_row.row_conflict_all > chain_group.row_conflict_all)
			chain_group.row_conflict_all = info_row.row_conflict_all;

		for (size_t col = 0; col < col_count && col < info_row.cell_conflict_this.size(); ++col)
		{
			if (info_row.cell_conflict_this[col] > chain_group.cell_conflict_this[col])
				chain_group.cell_conflict_this[col] = info_row.cell_conflict_this[col];
		}

		chain_group.children.push_back(std::move(info_row));
	}

	chain_group.all_identical = (chain_group.row_conflict_all <= conflict_all_t::no_conflict);
	m_rows.push_back(std::move(chain_group));
}

void view_tree_model_t::set_record_generic(record_context_t & context, const conflict_entry_t & entry)
{
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);

	if (entry.slot_result)
	{
		alignment_context_t align_ctx { all_subs, col_count, unified_slots, col_type_indices };
		content_alignment_t::build_from_slot_result(*entry.slot_result, align_ctx);
	}
	else
	{
		alignment_context_t align_ctx { all_subs, col_count, unified_slots, col_type_indices };
		content_alignment_t::build_occurrence_based(align_ctx);
	}

	for (const auto & slot : unified_slots)
		m_rows.push_back(build_slot_row(col_count, all_subs, col_type_indices, slot));
}

void view_tree_model_t::set_record_info(record_context_t & context, const conflict_entry_t & entry)
{
	(void)entry;
	const auto col_count = context.col_count;
	auto & all_subs = context.all_sub_records;

	std::vector<sub_slot_t> unified_slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);

	alignment_rule_t rule;
	rule.anchor_type = "SCVR";
	rule.anchor_size = 0;
	rule.trailing_types = { "INTV", "FLTV" };
	rule.key_source = alignment_rule_t::key_from_t::anchor;

	alignment_context_t align_ctx { all_subs, col_count, unified_slots, col_type_indices };
	content_alignment_t::align(align_ctx, { rule });

	int condition_index = 0;
	for (size_t i = 0; i < unified_slots.size(); ++i)
	{
		if (unified_slots[i].type != "SCVR")
		{
			m_rows.push_back(build_slot_row(col_count, all_subs, col_type_indices, unified_slots[i]));
			continue;
		}

		auto scvr_field = build_slot_row(col_count, all_subs, col_type_indices, unified_slots[i]);
		scvr_field.label = "SCVR - Script Variable";

		view_node_t group_row;
		group_row.type = "SCVR";
		group_row.size = 0;
		group_row.label = "Condition #" + std::to_string(condition_index);
		group_row.values.resize(col_count);
		group_row.show_group_value = true;
		group_row.all_identical = scvr_field.all_identical;

		std::vector<std::string> value_text(col_count);
		std::vector<view_node_t> value_fields;

		size_t next = i + 1;
		bool has_value = false;
		while (next < unified_slots.size() &&
		       (unified_slots[next].type == "INTV" || unified_slots[next].type == "FLTV"))
		{
			auto value_row = build_slot_row(col_count, all_subs, col_type_indices, unified_slots[next]);

			view_node_t value_field;
			value_field.label =
			    (unified_slots[next].type == "INTV") ? "INTV - Comparison Value" : "FLTV - Comparison Value";
			value_field.type = value_row.type;
			value_field.size = value_row.size;
			value_field.binary_ranges = value_row.binary_ranges;
			value_field.values = value_row.children.empty() ? value_row.values : value_row.children[0].values;
			value_field.all_identical = value_row.all_identical;
			value_field.row_conflict_all = value_row.row_conflict_all;
			value_field.cell_conflict_this = value_row.cell_conflict_this;

			if (!has_value)
			{
				for (size_t col = 0; col < col_count && col < value_field.values.size(); ++col)
					value_text[col] = value_field.values[col];

				has_value = true;
			}

			value_fields.push_back(std::move(value_field));
			++next;
		}

		for (size_t col = 0; col < col_count; ++col)
		{
			const auto * scvr_view = find_slot_view(all_subs, col_type_indices[col], unified_slots[i], col);
			const auto condition =
			    scvr_view ? parse_scvr_condition(scvr_view->data, scvr_view->size) : scvr_condition_t {};
			const auto sentence = format_scvr_condition(condition, value_text[col]);

			if (!sentence.empty())
				group_row.values[col] = sentence;
			else
				group_row.values[col] = scvr_field.values.size() > col ? scvr_field.values[col] : non_existent_value;
		}

		group_row.children.push_back(std::move(scvr_field));
		for (auto & value_field : value_fields)
			group_row.children.push_back(std::move(value_field));

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

		compute_group_ranges(group_row, col_count);

		m_rows.push_back(std::move(group_row));
		++condition_index;
		i = next - 1;
	}
}

void view_tree_model_t::collect_leveled_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	const std::string id_type = (m_record_type == "LEVC") ? "CNAM" : "INAM";

	alignment_rule_t rule;
	rule.anchor_type = id_type;
	rule.anchor_size = 0;
	rule.trailing_types = { "INTV" };
	rule.key_source = alignment_rule_t::key_from_t::anchor;

	alignment_context_t align_ctx { context.all_sub_records, context.col_count, build_ctx.unified_slots, build_ctx.col_type_indices };
	content_alignment_t::align(align_ctx, { rule });
}

void view_tree_model_t::collect_faction_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	alignment_rule_t rule;
	rule.anchor_type = "INTV";
	rule.anchor_size = 4;
	rule.trailing_types = { "ANAM" };
	rule.key_source = alignment_rule_t::key_from_t::next;
	rule.key_neighbor_type = "ANAM";

	alignment_context_t align_ctx { context.all_sub_records, context.col_count, build_ctx.unified_slots, build_ctx.col_type_indices };
	content_alignment_t::align(align_ctx, { rule });
}

void view_tree_model_t::collect_container_entries(record_context_t & context, slot_build_context_t & build_ctx)
{
	alignment_rule_t npco_rule;
	npco_rule.anchor_type = "NPCO";
	npco_rule.anchor_size = 36;
	npco_rule.key_source = alignment_rule_t::key_from_t::offset;
	npco_rule.key_offset = 4;
	npco_rule.key_length = 32;

	alignment_rule_t npcs_rule;
	npcs_rule.anchor_type = "NPCS";
	npcs_rule.anchor_size = 32;
	npcs_rule.key_source = alignment_rule_t::key_from_t::anchor;

	alignment_context_t align_ctx { context.all_sub_records, context.col_count, build_ctx.unified_slots, build_ctx.col_type_indices };
	content_alignment_t::align(align_ctx, { npco_rule, npcs_rule });
}

void view_tree_model_t::emit_slot_rows(record_context_t & context, slot_build_context_t & build_ctx)
{
	for (const auto & slot : build_ctx.unified_slots)
		m_rows.push_back(build_slot_row(context.col_count, context.all_sub_records, build_ctx.col_type_indices, slot));

	m_col_type_indices = build_ctx.col_type_indices;
}

void view_tree_model_t::emit_leveled_rows(record_context_t & context, slot_build_context_t & build_ctx)
{
	const auto & unified = build_ctx.unified_slots;
	const auto col_count = context.col_count;
	const bool is_creature = (m_record_type == "LEVC");
	const std::string id_type = is_creature ? "CNAM" : "INAM";
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

		view_node_t group_row;
		group_row.type = id_type;
		group_row.size = 0;
		group_row.values = id_row.values;
		group_row.all_identical = id_row.all_identical && level_row.all_identical;
		group_row.label = (is_creature ? "Creature #" : "Item #") + std::to_string(entry_index);

		view_node_t name_field;
		name_field.label = is_creature ? "CNAM - Creature Name" : "INAM - Item";
		name_field.type = id_row.type;
		name_field.size = id_row.size;
		name_field.values = id_row.values;
		name_field.binary_ranges = id_row.binary_ranges;
		name_field.all_identical = id_row.all_identical;
		name_field.row_conflict_all = id_row.row_conflict_all;
		name_field.cell_conflict_this = id_row.cell_conflict_this;

		view_node_t level_field;
		level_field.label = "INTV - PC Level";
		level_field.type = level_row.type;
		level_field.size = level_row.size;
		level_field.binary_ranges = level_row.binary_ranges;
		level_field.values = level_row.children.empty() ? level_row.values : level_row.children[0].values;
		level_field.all_identical = level_row.all_identical;
		level_field.row_conflict_all = level_row.row_conflict_all;
		level_field.cell_conflict_this = level_row.cell_conflict_this;

		group_row.children.push_back(std::move(name_field));
		group_row.children.push_back(std::move(level_field));

		group_row.row_conflict_all = std::max(id_row.row_conflict_all, level_row.row_conflict_all);
		group_row.cell_conflict_this.resize(col_count);
		for (size_t col = 0; col < col_count; ++col)
		{
			conflict_this_t id_ct =
			    (col < id_row.cell_conflict_this.size()) ? id_row.cell_conflict_this[col] : conflict_this_t::unknown;
			conflict_this_t lv_ct = (col < level_row.cell_conflict_this.size()) ? level_row.cell_conflict_this[col]
			                                                                    : conflict_this_t::unknown;
			group_row.cell_conflict_this[col] = std::max(id_ct, lv_ct);
		}

		compute_group_ranges(group_row, col_count);

		m_rows.push_back(std::move(group_row));
		++entry_index;
		++i;
	}

	m_col_type_indices = build_ctx.col_type_indices;
}
