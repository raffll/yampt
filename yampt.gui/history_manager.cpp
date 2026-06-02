#include "history_manager.hpp"
#include "editor_state.hpp"
#include "../yampt/json.hpp"
#include <chrono>
#include <ctime>
#include <fstream>

static std::string make_key(tools_t::rec_type_t type, const std::string & key)
{
    return tools_t::type2Str(type) + ":" + key;
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

void history_manager_t::record_change(tools_t::rec_type_t type, const std::string & key,
                                      const std::string & old_value, const std::string & new_value)
{
    auto compound_key = make_key(type, key);
    history_entry_t entry;
    entry.value = old_value;
    entry.timestamp = make_timestamp();
    entries_[compound_key].push_back(entry);
    session_modified_.insert(compound_key);
}

std::vector<history_entry_t> history_manager_t::get_history(tools_t::rec_type_t type,
                                                            const std::string & key) const
{
    auto compound_key = make_key(type, key);
    auto it = entries_.find(compound_key);
    if (it == entries_.end())
        return {};
    return it->second;
}

void history_manager_t::revert(editor_state_t & state, tools_t::rec_type_t type,
                               const std::string & key, size_t history_index)
{
    auto compound_key = make_key(type, key);
    auto it = entries_.find(compound_key);
    if (it == entries_.end())
        return;

    if (history_index >= it->second.size())
        return;

    const auto & entry = it->second[history_index];
    auto & dict = state.get_user_dict();
    auto type_it = dict.find(type);
    if (type_it == dict.end())
        return;

    auto * record = type_it->second.find(key);
    if (!record)
        return;

    std::string current_value = record->new_text;
    record->new_text = entry.value;

    auto index_it = type_it->second.index.find(key);
    if (index_it != type_it->second.index.end())
        state.mark_modified(type, index_it->second);

    record_change(type, key, current_value, entry.value);
}

void history_manager_t::load_from_file(const std::string & path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return;

    nlohmann::json j;
    file >> j;

    entries_.clear();
    for (auto & [key, arr] : j.items())
    {
        std::vector<history_entry_t> vec;
        for (auto & item : arr)
        {
            history_entry_t entry;
            entry.value = item.value("value", "");
            entry.timestamp = item.value("timestamp", "");
            vec.push_back(entry);
        }
        entries_[key] = std::move(vec);
    }
}

void history_manager_t::save_to_file(const std::string & path) const
{
    nlohmann::json j;
    for (const auto & [key, vec] : entries_)
    {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto & entry : vec)
        {
            nlohmann::json item;
            item["value"] = entry.value;
            item["timestamp"] = entry.timestamp;
            arr.push_back(item);
        }
        j[key] = arr;
    }

    std::ofstream file(path);
    if (!file.is_open())
        return;

    file << j.dump(2);
}

bool history_manager_t::is_modified_this_session(tools_t::rec_type_t type,
                                                 const std::string & key) const
{
    return session_modified_.count(make_key(type, key)) > 0;
}
