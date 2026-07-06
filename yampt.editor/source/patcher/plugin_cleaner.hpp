#pragma once

#include <functional>
#include <string>

class plugin_scan_t;

class plugin_cleaner_t
{
public:
	using log_fn_t = std::function<void(const std::string &)>;

	plugin_cleaner_t(plugin_scan_t & scan, log_fn_t log_fn);

	size_t remove_itm_records(int plugin_idx);

private:
	plugin_scan_t & m_scan;
	log_fn_t m_log;
};
