#include "decoder/content_alignment.hpp"
#include <decoder/conflict_slots.hpp>
#include <algorithm>

std::string content_alignment_t::extract_key(
    const sub_record_view_t & anchor,
    const std::vector<sub_record_view_t> & subs,
    size_t anchor_pos,
    const alignment_rule_t & rule)
{
	if (rule.key_source == alignment_rule_t::key_from_t::offset)
	{
		if (rule.key_offset + rule.key_length > anchor.size)
			return std::string(anchor.data, anchor.size);

		std::string key(anchor.data + rule.key_offset, rule.key_length);
		while (!key.empty() && key.back() == '\0')
			key.pop_back();

		return key;
	}

	if (rule.key_source == alignment_rule_t::key_from_t::next)
	{
		if (anchor_pos + 1 < subs.size() && subs[anchor_pos + 1].type == rule.key_neighbor_type)
		{
			std::string key(subs[anchor_pos + 1].data, subs[anchor_pos + 1].size);
			while (!key.empty() && key.back() == '\0')
				key.pop_back();

			return key;
		}
	}

	return std::string(anchor.data, anchor.size);
}

void content_alignment_t::collect_non_excluded(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    const std::vector<alignment_rule_t> & rules,
    std::vector<sub_slot_t> & unified_slots,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	auto is_excluded = [&rules](const sub_record_view_t & sv_rec) -> bool
	{
		for (const auto & rule : rules)
		{
			if (sv_rec.type == rule.anchor_type && (rule.anchor_size == 0 || sv_rec.size == rule.anchor_size))
				return true;

			for (const auto & trailing_type : rule.trailing_types)
			{
				if (sv_rec.type == trailing_type)
					return true;
			}
		}

		return false;
	};

	std::unordered_map<std::string, int> type_count;
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (const auto & sv_rec : all_subs[col])
		{
			if (is_excluded(sv_rec))
				continue;

			int occur = type_count[sv_rec.type]++;
			bool found = false;
			for (const auto & slot : unified_slots)
			{
				if (slot.type == sv_rec.type && slot.occurrence == occur)
				{
					found = true;
					break;
				}
			}

			if (!found)
				unified_slots.push_back({ sv_rec.type, occur });
		}

		type_count.clear();
	}

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (size_t i = 0; i < all_subs[col].size(); ++i)
		{
			if (is_excluded(all_subs[col][i]))
				continue;

			col_type_indices[col][all_subs[col][i].type].push_back(i);
		}
	}
}

void content_alignment_t::scan_groups(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    const alignment_rule_t & rule,
    std::vector<std::vector<aligned_group_t>> & col_groups,
    std::vector<std::string> & all_keys,
    int merge_column)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		const auto & subs = all_subs[col];
		for (size_t i = 0; i < subs.size(); ++i)
		{
			if (subs[i].type != rule.anchor_type)
				continue;

			if (rule.anchor_size > 0 && subs[i].size != rule.anchor_size)
				continue;

			const auto key = extract_key(subs[i], subs, i, rule);

			aligned_group_t group;
			group.content_key = key;
			group.anchor_idx = i;

			for (size_t j = i + 1; j < subs.size(); ++j)
			{
				bool is_trailing = false;
				for (const auto & trailing_type : rule.trailing_types)
				{
					if (subs[j].type == trailing_type)
					{
						is_trailing = true;
						break;
					}
				}

				if (!is_trailing)
					break;

				group.trailing.push_back({ subs[j].type, j });
			}

			col_groups[col].push_back(std::move(group));

			if (static_cast<int>(col) != merge_column)
			{
				if (std::find(all_keys.begin(), all_keys.end(), key) == all_keys.end())
					all_keys.push_back(key);
			}
		}
	}
}

void content_alignment_t::emit_key_slots(
    const std::string & key,
    const std::vector<std::vector<aligned_group_t>> & col_groups,
    size_t col_count,
    const alignment_rule_t & rule,
    std::vector<sub_slot_t> & unified_slots,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices,
    int merge_column)
{
	size_t max_trailing = 0;
	for (size_t col = 0; col < col_count; ++col)
	{
		for (const auto & group : col_groups[col])
		{
			if (group.content_key == key)
			{
				max_trailing = std::max(max_trailing, group.trailing.size());
				break;
			}
		}
	}

	int anchor_occ = -1;
	for (const auto & slot : unified_slots)
	{
		if (slot.type == rule.anchor_type)
			anchor_occ = std::max(anchor_occ, slot.occurrence);
	}

	unified_slots.push_back({ rule.anchor_type, anchor_occ + 1 });

	std::vector<std::string> trailing_slot_types;
	for (size_t trailing = 0; trailing < max_trailing; ++trailing)
	{
		std::string trailing_type = resolve_trailing_type(key, trailing, col_groups, col_count, rule);

		int trail_occ = -1;
		for (const auto & slot : unified_slots)
		{
			if (slot.type == trailing_type)
				trail_occ = std::max(trail_occ, slot.occurrence);
		}

		unified_slots.push_back({ trailing_type, trail_occ + 1 });
		trailing_slot_types.push_back(trailing_type);
	}

	fill_key_indices(key, col_groups, col_count, rule, max_trailing, trailing_slot_types, col_type_indices, merge_column);
}

std::string content_alignment_t::resolve_trailing_type(
    const std::string & key,
    size_t trailing_index,
    const std::vector<std::vector<aligned_group_t>> & col_groups,
    size_t col_count,
    const alignment_rule_t & rule)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		for (const auto & group : col_groups[col])
		{
			if (group.content_key != key)
				continue;

			if (trailing_index < group.trailing.size())
				return group.trailing[trailing_index].first;
		}
	}

	return rule.trailing_types.empty() ? "DATA" : rule.trailing_types[0];
}

void content_alignment_t::fill_key_indices(
    const std::string & key,
    const std::vector<std::vector<aligned_group_t>> & col_groups,
    size_t col_count,
    const alignment_rule_t & rule,
    size_t max_trailing,
    const std::vector<std::string> & trailing_slot_types,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices,
    int merge_column)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (static_cast<int>(col) == merge_column)
			continue;
		bool found = false;
		for (const auto & group : col_groups[col])
		{
			if (group.content_key != key)
				continue;

			col_type_indices[col][rule.anchor_type].push_back(group.anchor_idx);
			for (size_t trailing = 0; trailing < max_trailing; ++trailing)
			{
				if (trailing < group.trailing.size())
					col_type_indices[col][trailing_slot_types[trailing]].push_back(group.trailing[trailing].second);
				else
					col_type_indices[col][trailing_slot_types[trailing]].push_back(SIZE_MAX);
			}

			found = true;
			break;
		}

		if (!found)
		{
			col_type_indices[col][rule.anchor_type].push_back(SIZE_MAX);
			for (size_t trailing = 0; trailing < max_trailing; ++trailing)
				col_type_indices[col][trailing_slot_types[trailing]].push_back(SIZE_MAX);
		}
	}
}

void content_alignment_t::align(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    const std::vector<alignment_rule_t> & rules,
    std::vector<sub_slot_t> & unified_slots,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	collect_non_excluded(all_subs, col_count, rules, unified_slots, col_type_indices);

	for (const auto & rule : rules)
	{
		std::vector<std::vector<aligned_group_t>> col_groups(col_count);
		std::vector<std::string> all_keys;

		scan_groups(all_subs, col_count, rule, col_groups, all_keys, -1);

		for (const auto & key : all_keys)
			emit_key_slots(key, col_groups, col_count, rule, unified_slots, col_type_indices, -1);
	}
}

void content_alignment_t::fit_merge_column(
    const std::vector<std::vector<sub_record_view_t>> &,
    const std::vector<std::vector<aligned_group_t>> & col_groups,
    const std::vector<std::string> & all_keys,
    int merge_column,
    const alignment_rule_t & rule,
    const std::vector<sub_slot_t> & unified_slots,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	const size_t merge_col = static_cast<size_t>(merge_column);
	if (merge_col >= col_groups.size())
		return;

	const auto & merge_groups = col_groups[merge_col];

	size_t slot_cursor = 0;
	for (const auto & key : all_keys)
	{
		while (slot_cursor < unified_slots.size() && unified_slots[slot_cursor].type != rule.anchor_type)
			++slot_cursor;

		if (slot_cursor >= unified_slots.size())
			break;

		size_t trailing_count = 0;
		for (size_t t = slot_cursor + 1; t < unified_slots.size(); ++t)
		{
			bool is_trailing = false;
			for (const auto & trail_type : rule.trailing_types)
			{
				if (unified_slots[t].type == trail_type)
				{
					is_trailing = true;
					break;
				}
			}

			if (!is_trailing)
				break;

			++trailing_count;
		}

		bool found = false;
		for (const auto & group : merge_groups)
		{
			if (group.content_key != key)
				continue;

			col_type_indices[merge_col][rule.anchor_type].push_back(group.anchor_idx);
			for (size_t trailing = 0; trailing < trailing_count; ++trailing)
			{
				const auto & trail_type = unified_slots[slot_cursor + 1 + trailing].type;
				if (trailing < group.trailing.size())
					col_type_indices[merge_col][trail_type].push_back(group.trailing[trailing].second);
				else
					col_type_indices[merge_col][trail_type].push_back(SIZE_MAX);
			}

			found = true;
			break;
		}

		if (!found)
		{
			col_type_indices[merge_col][rule.anchor_type].push_back(SIZE_MAX);
			for (size_t trailing = 0; trailing < trailing_count; ++trailing)
			{
				const auto & trail_type = unified_slots[slot_cursor + 1 + trailing].type;
				col_type_indices[merge_col][trail_type].push_back(SIZE_MAX);
			}
		}

		slot_cursor += 1 + trailing_count;
	}
}

void content_alignment_t::build_from_slot_result(
    const slot_result_t & slot_result,
    size_t col_count,
    std::vector<sub_slot_t> & unified_slots,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	for (const auto & aligned_slot : slot_result.aligned)
		unified_slots.push_back({ aligned_slot.key.type, aligned_slot.key.occurrence });

	for (const auto & aligned_slot : slot_result.aligned)
	{
		const auto ver_size = aligned_slot.indices.size();
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col < ver_size)
				col_type_indices[col][aligned_slot.key.type].push_back(aligned_slot.indices[col]);
			else
				col_type_indices[col][aligned_slot.key.type].push_back(SIZE_MAX);
		}
	}
}

void content_alignment_t::build_occurrence_based(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    std::vector<sub_slot_t> & unified_slots,
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices)
{
	for (const auto & subs : all_subs)
	{
		std::unordered_map<std::string, int> type_count;
		for (const auto & sv_rec : subs)
		{
			int occur = type_count[sv_rec.type]++;
			bool found = false;
			for (const auto & slot : unified_slots)
			{
				if (slot.type == sv_rec.type && slot.occurrence == occur)
				{
					found = true;
					break;
				}
			}

			if (!found)
				unified_slots.push_back({ sv_rec.type, occur });
		}
	}

	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size())
			continue;

		for (size_t i = 0; i < all_subs[col].size(); ++i)
			col_type_indices[col][all_subs[col][i].type].push_back(i);
	}
}

void content_alignment_t::build_occurrence_from_ranges(
    const std::vector<std::vector<sub_record_view_t>> & all_subs,
    size_t col_count,
    const std::vector<size_t> & col_start,
    const std::vector<size_t> & col_end,
    std::vector<sub_slot_t> & unified_slots)
{
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_subs.size() || col >= col_start.size() || col >= col_end.size())
			continue;

		std::unordered_map<std::string, int> type_count;
		for (size_t i = col_start[col]; i < col_end[col]; ++i)
		{
			const auto & sv_rec = all_subs[col][i];
			int occur = type_count[sv_rec.type]++;
			bool found = false;
			for (const auto & slot : unified_slots)
			{
				if (slot.type == sv_rec.type && slot.occurrence == occur)
				{
					found = true;
					break;
				}
			}

			if (!found)
				unified_slots.push_back({ sv_rec.type, occur });
		}
	}
}
