#pragma once

#include "conflict_enums.hpp"
#include "plugin_index.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class plugin_scan_t;

struct record_version_t
{
	int plugin_idx;
	size_t record_index;
	conflict_this_t status = conflict_this_t::unknown;
};

struct conflict_entry_t
{
	std::string rec_type;
	std::string record_id;
	std::string display_name;
	std::string dial_name;
	conflict_all_t conflict_all = conflict_all_t::unknown;
	std::vector<record_version_t> versions;
	std::unique_ptr<slot_result_t> slot_result;
};

class record_conflicts_t
{
public:
	explicit record_conflicts_t(plugin_scan_t & scan);

	void rebuild();
	void include_merge_records(int merge_plugin_idx, const std::vector<std::string> & merge_types,
	                           const std::vector<std::string> & merge_ids,
	                           const std::vector<std::string> & merge_contents);

	const std::vector<conflict_entry_t> & entries() const;
	const conflict_entry_t * find(const std::string & type, const std::string & id) const;
	std::vector<std::string> all_types() const;

	size_t itm_count(int plugin_idx) const;
	std::vector<const conflict_entry_t *> itm_entries(int plugin_idx) const;

private:
	struct version_descriptor_t
	{
		std::string rec_type;
		std::string record_id;
		std::string display_name;
		std::string dial_name;
		record_version_t version;
	};

	void insert_or_update_version(version_descriptor_t desc);
	void compute_conflict(conflict_entry_t & entry);

	plugin_scan_t & m_scan;
	std::vector<conflict_entry_t> m_entries;
	std::unordered_map<std::string, size_t> m_entry_lookup;
};
