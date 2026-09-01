#include "edit_log.hpp"
#include <chrono>
#include <ctime>

static std::string make_timestamp()
{
	const auto now = std::chrono::system_clock::now();
	const auto time = std::chrono::system_clock::to_time_t(now);
	struct tm tm_buf;
#ifdef _WIN32
	localtime_s(&tm_buf, &time);
#else
	localtime_r(&time, &tm_buf);
#endif
	char buffer[16];
	std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm_buf);
	return std::string(buffer);
}

void edit_log_t::record_field_edit(const field_edit_record_t & edit)
{
	std::string description = "edited " + edit.record_type + ":" + edit.record_id;
	if (!edit.field_name.empty())
		description += " " + edit.field_name;

	description += " = " + edit.new_value;

	append(edit.plugin_filename, description);
}

void edit_log_t::record_record_removal(const record_removal_record_t & removal)
{
	const std::string description = "removed " + removal.record_type + ":" + removal.record_id;
	append(removal.plugin_filename, description);
}

void edit_log_t::clear()
{
	m_entries.clear();
}

const std::vector<edit_log_entry_t> & edit_log_t::entries() const
{
	return m_entries;
}

void edit_log_t::append(const std::string & plugin_filename, const std::string & description)
{
	edit_log_entry_t entry;
	entry.timestamp = make_timestamp();
	entry.plugin_filename = plugin_filename;
	entry.description = description;
	m_entries.push_back(std::move(entry));
}
