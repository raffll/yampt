#include "plugin_cleaner.hpp"
#include <scanner/plugin_scan.hpp>

plugin_cleaner_t::plugin_cleaner_t(plugin_scan_t & scan, log_fn_t log_fn)
    : m_scan(scan)
    , m_log(std::move(log_fn))
{}

size_t plugin_cleaner_t::remove_itm_records(int plugin_idx)
{
	const auto itms = m_scan.itm_entries(plugin_idx);
	const auto count_before = m_scan.merge_record_count();

	for (const auto * entry : itms)
		m_scan.remove_from_merge(entry->rec_type, entry->record_id);

	const auto removed = count_before - m_scan.merge_record_count();
	if (removed > 0)
		m_log("Removed " + std::to_string(removed) + " ITM records from " + m_scan.plugin_filename(plugin_idx));

	return removed;
}
