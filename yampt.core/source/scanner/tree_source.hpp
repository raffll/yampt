#pragma once

#include "conflict_enums.hpp"
#include "plugin_scan.hpp"
#include <utility/plugin_icon_tiers.hpp>
#include <span>
#include <string>

struct plugin_meta_t
{
	std::string filename;
	std::string path;
	bool is_guard = false;
	bool is_active = false;
};

struct foreground_query_t
{
	const conflict_entry_t & entry;
	int plugin_idx = -1;
	bool hide_duplicates = false;
};

struct group_query_t
{
	const plugin_scan_t & scan;
	std::span<const size_t> entry_indices;
	int plugin_idx = -1;
	bool hide_duplicates = false;
};

struct group_conflict_t
{
	conflict_all_t worst_all = conflict_all_t::only_one;
	conflict_this_t worst_this = conflict_this_t::unknown;
};

class tree_source_t
{
public:
	static plugin_icon::tier_flags_t icon_flags(const plugin_meta_t & meta);

	static conflict_this_t record_foreground_class(const foreground_query_t & query);
	static conflict_this_t accumulate_worst_this(conflict_this_t worst, conflict_this_t status);
	static conflict_this_t resolve_foreground_class(conflict_this_t worst);
	static size_t unique_plugin_count(const conflict_entry_t & entry);
	static group_conflict_t worst_group_conflict(const group_query_t & query);
};
