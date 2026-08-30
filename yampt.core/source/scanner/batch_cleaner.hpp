#pragma once

#include <functional>
#include <string>
#include <vector>

class plugin_scan_t;

struct clean_options_t
{
	bool evil_gmst = true;
	bool junk_cell = true;
	bool update_master_sizes = false;
	bool update_version = false;
};

struct clean_result_t
{
	std::string plugin_filename;
	int evil_gmst_removed = 0;
	int junk_cell_removed = 0;
	int total_removed = 0;
	int master_sizes_updated = 0;
	bool version_updated = false;
	bool written = false;
};

class batch_cleaner_t
{
public:
	using log_fn_t = std::function<void(const std::string &)>;

	batch_cleaner_t(plugin_scan_t & scan, log_fn_t log_fn);

	void set_options(const clean_options_t & options);
	std::vector<clean_result_t> clean_all(const std::string & output_directory);

	static bool is_evil_gmst(const std::string & record_id, const std::string & record_content);
	static bool is_junk_cell(const std::string & record_content);

private:
	clean_result_t clean_plugin(int plugin_idx, const std::string & output_directory);
	bool is_master_plugin(int plugin_idx) const;
	std::string resolve_plugin_directory(int plugin_idx) const;

	plugin_scan_t & m_scan;
	log_fn_t m_log;
	clean_options_t m_options;
};
