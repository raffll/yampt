#pragma once

#include "sub_record_merge.hpp"
#include <set>
#include <string>
#include <vector>

class plugin_scan_t;

struct merge_config_t
{
	std::set<std::string> excluded_plugins;
	std::string exclusion_pattern;
	std::set<std::string> disabled_types;
	bool fog_fix_enabled = true;
	bool summon_fix_enabled = true;
	bool cell_name_fix_enabled = true;
};

struct merge_counters_t
{
	int three_way = 0;
	int lists = 0;
	int dialogues = 0;
	int fixes = 0;
};

struct merge_log_entry_t
{
	std::string message;
};

class merge_compute_t
{
public:
	explicit merge_compute_t(plugin_scan_t & scan);

	void set_config(const merge_config_t & config);
	merge_counters_t execute();

	const std::vector<merge_log_entry_t> & log_entries() const;

private:
	struct version_ref_t
	{
		int plugin_idx;
		size_t record_index;
	};

	struct record_group_t
	{
		std::string rec_type;
		std::string record_id;
		std::vector<version_ref_t> versions;
	};

	void build_record_groups();
	void process_groups(merge_counters_t & counters);
	void process_leveled_list(const record_group_t & group, merge_counters_t & counters);
	void process_dialogue(const record_group_t & group, merge_counters_t & counters);
	void process_three_way(const record_group_t & group, merge_counters_t & counters);

	void apply_fixes(merge_counters_t & counters);
	void apply_fog_fixes(merge_counters_t & counters);
	void apply_summon_fixes(merge_counters_t & counters);
	void apply_cell_name_fixes(merge_counters_t & counters);

	void prune_unchanged();

	std::vector<std::string> read_version_contents(const record_group_t & group);

	bool is_plugin_included(int plugin_idx) const;
	bool is_type_enabled(const std::string & rec_type) const;
	bool matches_exclusion(const std::string & record_id) const;

	void add_log(const std::string & message);

	plugin_scan_t & m_scan;
	merge_config_t m_config;
	std::vector<record_group_t> m_groups;
	std::vector<merge_log_entry_t> m_log;
};
