#pragma once

#include "../yampt/tools.hpp"
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct dict_slot_t;

struct history_entry_t
{
	std::string value;
	std::string timestamp;
};

class history_manager_t
{
public:
	void record_change(
	    tools_t::rec_type_t type,
	    const std::string & key,
	    const std::string & old_value,
	    const std::string & new_value);
	std::vector<history_entry_t> get_history(tools_t::rec_type_t type, const std::string & key) const;
	void revert(dict_slot_t & slot, tools_t::rec_type_t type, const std::string & key, size_t history_index);

	void load_from_file(const std::string & path);
	void save_to_file(const std::string & path) const;

	bool is_modified_this_session(tools_t::rec_type_t type, const std::string & key) const;

private:
	std::unordered_map<std::string, std::vector<history_entry_t>> entries_;
	std::set<std::string> session_modified_;
};
