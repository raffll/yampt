#include "edit_history.hpp"
#include <nlohmann/json.hpp>
#include <utility/status_types.hpp>
#include <chrono>
#include <ctime>
#include <fstream>

static std::string make_key(rec_type_t type, const std::string & key)
{
	return domain_types::type_to_str(type) + ":" + key;
}

static std::string make_timestamp()
{
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	struct tm tm_buf;
	localtime_s(&tm_buf, &time);
	char buf[20];
	std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);
	return std::string(buf);
}

void edit_history_t::record_change(
    rec_type_t type,
    const std::string & key,
    const std::string & old_value,
    const std::string & new_value,
    status_t old_status)
{
	auto compound_key = make_key(type, key);
	history_entry_t entry;
	entry.value = old_value;
	entry.timestamp = make_timestamp();
	entry.status = old_status;
	m_entries[compound_key].push_back(entry);
	m_session_modified.insert(compound_key);
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

void edit_history_t::load_from_file(const std::string & path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return;

	nlohmann::json j;
	file >> j;

	m_entries.clear();
	for (auto & [key, arr] : j.items())
	{
		std::vector<history_entry_t> vec;
		for (auto & item : arr)
		{
			history_entry_t entry;
			entry.value = item.value("value", "");
			entry.timestamp = item.value("timestamp", "");
			entry.status = string_to_status(item.value("status", "untranslated"));
			vec.push_back(entry);
		}
		m_entries[key] = std::move(vec);
	}
}

void edit_history_t::save_to_file(const std::string & path) const
{
	nlohmann::json j;
	for (const auto & [key, vec] : m_entries)
	{
		nlohmann::json arr = nlohmann::json::array();
		for (const auto & entry : vec)
		{
			nlohmann::json item;
			item["value"] = entry.value;
			item["timestamp"] = entry.timestamp;
			item["status"] = std::string(status_to_string(entry.status));
			arr.push_back(item);
		}
		j[key] = arr;
	}

	std::ofstream file(path);
	if (!file.is_open())
		return;

	file << j.dump(2);
}

bool edit_history_t::is_modified_this_session(rec_type_t type, const std::string & key) const
{
	return m_session_modified.count(make_key(type, key)) > 0;
}
