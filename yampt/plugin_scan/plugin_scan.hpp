#pragma once

#include "plugin_index.hpp"
#include "conflict_types.hpp"
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
	std::vector<record_version_t> versions;
};

class plugin_scan_t
{
public:
	void load_plugin(const std::string & path);
	void set_merge_plugin(const std::string & filename);
	void rebuild_conflicts();

	size_t plugin_count() const;
	const std::string & plugin_path(int idx) const;
	std::string plugin_filename(int idx) const;
	const esm_reader_t & plugin(int idx) const;
	const plugin_index_t & index(int idx) const;
	bool is_merge_plugin(int idx) const;

	const std::vector<conflict_entry_t> & entries() const;
	const conflict_entry_t * find(const std::string & type, const std::string & id) const;
	std::vector<std::string> all_types() const;

	void copy_record_to_merge(int source_plugin, size_t record_index);
	void remove_from_merge(const std::string & type, const std::string & id);
	bool save_merge(const std::string & output_path, const std::string & author, const std::string & description);
	bool has_merge() const;
	size_t merge_record_count() const;

	size_t itm_count(int plugin_idx) const;
	std::vector<const conflict_entry_t *> itm_entries(int plugin_idx) const;

private:
	void compute_conflict(conflict_entry_t & entry);
	bool content_equal(const std::string & a, const std::string & b) const;
	std::string build_tes3_header(const std::string & author, const std::string & description);

	struct loaded_plugin_t
	{
		loaded_plugin_t() = default;
		explicit loaded_plugin_t(const std::string & file_path)
		    : path(file_path)
		    , esm(file_path)
		    , index(esm)
		{
		}

		std::string path;
		esm_reader_t esm;
		plugin_index_t index{esm};
	};

	std::vector<std::unique_ptr<loaded_plugin_t>> plugins_;
	int merge_plugin_idx_ = -1;

	struct merge_record_t
	{
		std::string rec_type;
		std::string record_id;
		std::string content;
	};
	std::vector<merge_record_t> merge_records_;

	std::vector<conflict_entry_t> entries_;
	std::unordered_map<std::string, size_t> entry_lookup_;
};
