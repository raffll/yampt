#include "tree_source.hpp"
#include <set>

plugin_icon::tier_flags_t tree_source_t::icon_flags(const plugin_meta_t & meta)
{
	plugin_icon::tier_flags_t flags;
	flags.filename = meta.filename;
	flags.is_overridden = plugin_icon::path_is_overwrite(meta.path);
	flags.is_guard = meta.is_guard;
	flags.is_active = meta.is_active;

	return flags;
}

conflict_this_t tree_source_t::record_foreground_class(const foreground_query_t & query)
{
	const auto & entry = query.entry;

	if (entry.versions.size() <= 1)
		return conflict_this_t::unknown;

	if (query.hide_duplicates && unique_plugin_count(entry) <= 1)
		return conflict_this_t::unknown;

	for (const auto & version : entry.versions)
	{
		if (version.plugin_idx != query.plugin_idx)
			continue;

		return version.status;
	}

	return conflict_this_t::unknown;
}

conflict_this_t tree_source_t::accumulate_worst_this(conflict_this_t worst, conflict_this_t status)
{
	if (status == conflict_this_t::identical_to_master)
		return worst;

	if (status > worst)
		return status;

	return worst;
}

conflict_this_t tree_source_t::resolve_foreground_class(conflict_this_t worst)
{
	if (worst == conflict_this_t::unknown || worst == conflict_this_t::master)
		return conflict_this_t::identical_to_master;

	return worst;
}

size_t tree_source_t::unique_plugin_count(const conflict_entry_t & entry)
{
	std::set<int> plugins;
	for (const auto & version : entry.versions)
		plugins.insert(version.plugin_idx);

	return plugins.size();
}

group_conflict_t tree_source_t::worst_group_conflict(const group_query_t & query)
{
	group_conflict_t result;

	for (const auto entry_idx : query.entry_indices)
	{
		const auto & entry = query.entries[entry_idx];
		const auto this_class =
		    record_foreground_class({ entry, query.plugin_idx, query.hide_duplicates });

		result.worst_this = accumulate_worst_this(result.worst_this, this_class);

		if (entry.conflict_all > result.worst_all)
			result.worst_all = entry.conflict_all;
	}

	return result;
}
