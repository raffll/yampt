#pragma once

#include <utility/status_types.hpp>
#include <utility/tools.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct history_entry_t
{
	std::string value;
	std::string timestamp;
	status_t status = status_t::untranslated;
};

struct revert_result_t
{
	std::string reverted_text;
	status_t reverted_status = status_t::untranslated;
	bool success = false;
};

class edit_history_t
{
public:
	void record_change(
	    tools_t::rec_type_t type,
	    const std::string & key,
	    const std::string & old_value,
	    const std::string & new_value,
	    status_t old_status);
	std::vector<history_entry_t> get_history(tools_t::rec_type_t type, const std::string & key) const;
	revert_result_t revert(tools_t::rec_type_t type, const std::string & key, size_t history_index);

	void load_from_file(const std::string & path);
	void save_to_file(const std::string & path) const;

	bool is_modified_this_session(tools_t::rec_type_t type, const std::string & key) const;

private:
	std::unordered_map<std::string, std::vector<history_entry_t>> m_entries;
	std::set<std::string> m_session_modified;
};
