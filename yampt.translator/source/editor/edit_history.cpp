#include "edit_history.hpp"
#include <utility/status_types.hpp>
#include <chrono>
#include <ctime>

static std::string make_key(rec_type_t type, const std::string & key)
{
	return domain_types::type_to_str(type) + ":" + key;
}

static std::string make_timestamp()
{
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	struct tm tm_buf;
#ifdef _WIN32
	localtime_s(&tm_buf, &time);
#else
	localtime_r(&time, &tm_buf);
#endif
	char buf[20];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
	return std::string(buf);
}

void edit_history_t::record_change(
    rec_type_t type,
    const std::string & key,
    const std::string & old_value,
    const std::string & new_value,
    status_t old_status)
{
	(void)new_value;
	auto compound_key = make_key(type, key);
	history_entry_t entry;
	entry.value = old_value;
	entry.timestamp = make_timestamp();
	entry.status = old_status;
	m_entries[compound_key].push_back(entry);
}

std::vector<history_entry_t> edit_history_t::get_history(rec_type_t type, const std::string & key) const
{
	auto compound_key = make_key(type, key);
	auto it = m_entries.find(compound_key);
	if (it == m_entries.end())
		return {};
	return it->second;
}

revert_result_t edit_history_t::revert(rec_type_t type, const std::string & key, size_t history_index)
{
	auto compound_key = make_key(type, key);
	auto it = m_entries.find(compound_key);
	if (it == m_entries.end())
		return {};

	if (history_index >= it->second.size())
		return {};

	const auto & entry = it->second[history_index];
	return { entry.value, entry.status, true };
}
