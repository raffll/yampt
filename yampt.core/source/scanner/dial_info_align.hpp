#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct info_align_entry_t
{
	std::string inam;
	std::string display_name;
	std::vector<bool> present_in_plugin;
};

struct dial_info_align_result_t
{
	std::string dial_record_id;
	std::vector<std::string> plugin_names;
	std::vector<info_align_entry_t> entries;
};

class plugin_scan_t;

class dial_info_align_t
{
public:
	static dial_info_align_result_t build(const plugin_scan_t & scan, const std::string & dial_record_id);
};
