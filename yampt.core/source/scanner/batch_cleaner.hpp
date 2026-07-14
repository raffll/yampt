#pragma once

#include <functional>
#include <string>
#include <vector>

class plugin_scan_t;

struct clean_result_t
{
	std::string plugin_filename;
	int itm_removed = 0;
	int evil_gmst_removed = 0;
	int junk_cell_removed = 0;
	int total_removed = 0;
	bool written = false;
};

class batch_cleaner_t
{
public:
	using log_fn_t = std::function<void(const std::string &)>;

	batch_cleaner_t(plugin_scan_t & scan, log_fn_t log_fn);

	std::vector<clean_result_t> clean_all(const std::string & output_directory);

	static bool is_evil_gmst(const std::string & record_id, const std::string & record_content);
	static bool is_junk_cell(const std::string & record_content);

private:
	clean_result_t clean_plugin(int plugin_idx, const std::string & output_directory);
	bool is_master_plugin(int plugin_idx) const;

	plugin_scan_t & m_scan;
	log_fn_t m_log;
};
