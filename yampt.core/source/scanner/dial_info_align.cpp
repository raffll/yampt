#include "dial_info_align.hpp"
#include "plugin_scan.hpp"

dial_info_align_result_t dial_info_align_t::build(
    const plugin_scan_t & scan,
    const std::string & dial_record_id)
{
	dial_info_align_result_t result;
	result.dial_record_id = dial_record_id;

	const size_t plugin_count = scan.plugin_count();
	for (size_t i = 0; i < plugin_count; ++i)
		result.plugin_names.push_back(scan.plugin_filename(static_cast<int>(i)));

	const auto & all_entries = scan.entries();

	for (const auto & entry : all_entries)
	{
		if (entry.rec_type != "INFO")
			continue;

		if (entry.dial_name != dial_record_id)
			continue;

		auto separator_pos = entry.record_id.find('|');
		std::string inam = (separator_pos != std::string::npos)
		    ? entry.record_id.substr(separator_pos + 1)
		    : entry.record_id;

		info_align_entry_t align_entry;
		align_entry.inam = inam;
		align_entry.display_name = entry.display_name;
		align_entry.present_in_plugin.resize(plugin_count, false);

		for (const auto & version : entry.versions)
		{
			if (version.plugin_idx >= 0 && version.plugin_idx < static_cast<int>(plugin_count))
				align_entry.present_in_plugin[version.plugin_idx] = true;
		}

		result.entries.push_back(std::move(align_entry));
	}

	return result;
}
