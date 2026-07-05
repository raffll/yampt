#pragma once

#include "../decoder/conflict_slots.hpp"
#include "conflict_enums.hpp"
#include "plugin_index.hpp"
#include <memory>
#include <string>
#include <vector>

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
	bool has_dele = false;
	std::vector<record_version_t> versions;
	std::unique_ptr<slot_result_t> slot_result;
};

class plugin_scan_t
{
public:
	struct merge_record_t
	{
		std::string rec_type;
		std::string record_id;
		std::string content;
		bool pinned = false;
	};

	void load_plugin(const std::string & path);
	void set_merge_plugin(const std::string & filename);
	void set_merge_plugin_from_loaded(int plugin_idx);
	void clear_merge_records();
	std::vector<merge_record_t> collect_pinned_records() const;
	void restore_pinned_records(const std::vector<merge_record_t> & pinned);
	void rebuild_conflicts();

	size_t plugin_count() const;
	const std::string & plugin_path(int idx) const;
	std::string plugin_filename(int idx) const;
	const esm_reader_t & plugin(int idx) const;
	const plugin_index_t & index(int idx) const;
	bool is_merge_plugin(int idx) const;
	const std::vector<std::string> & master_list(int idx) const;
	uint64_t resolve_frmr(int plugin_idx, uint32_t raw_frmr) const;

	const std::vector<conflict_entry_t> & entries() const;
	const conflict_entry_t * find(const std::string & type, const std::string & id) const;
	std::vector<std::string> all_types() const;

	void copy_record_to_merge(int source_plugin, size_t record_index);
	void copy_record_to_merge_raw(
	    const std::string & rec_type,
	    const std::string & record_id,
	    const std::string & content);
	void pin_record_to_merge(const std::string & rec_type, const std::string & record_id, const std::string & content);
	bool is_merge_pinned(const std::string & rec_type, const std::string & record_id) const;
	const std::string * find_merge_content(const std::string & rec_type, const std::string & record_id) const;
	std::string read_record_content(int plugin_idx, size_t record_index);
	void remove_from_merge(const std::string & type, const std::string & id);
	void recompute_single_conflict(const std::string & rec_type, const std::string & record_id);
	bool has_merge() const;
	size_t merge_record_count() const;
	const std::string & merge_record_content(size_t index) const;
	const std::string & merge_record_type(size_t index) const;
	const std::string & merge_record_id(size_t index) const;

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

	void compute_conflict(conflict_entry_t & entry);
	void insert_or_update_version(version_descriptor_t desc);

	struct loaded_plugin_t
	{
		loaded_plugin_t() = default;

		explicit loaded_plugin_t(const std::string & file_path)
		    : path(file_path)
		    , esm(file_path)
		    , index(esm)
		{
			parse_master_list();
		}

		std::string path;
		esm_reader_t esm;
		plugin_index_t index { esm };
		std::vector<std::string> master_files;

	private:
		void parse_master_list();
	};

	std::vector<std::unique_ptr<loaded_plugin_t>> m_plugins;
	int m_merge_plugin_idx = -1;

	std::vector<merge_record_t> m_merge_records;

	std::vector<conflict_entry_t> m_entries;
	std::unordered_map<std::string, size_t> m_entry_lookup;
};
