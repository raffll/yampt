#pragma once

#include "../yampt/tools.hpp"
#include <set>
#include <string>
#include <utility>
#include <vector>

struct filter_state_t
{
	std::set<tools_t::rec_type_t> type_filter = {
		tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
		tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
		tools_t::rec_type_t::indx, tools_t::rec_type_t::bnam, tools_t::rec_type_t::sctx,
	};
	std::set<std::string> status_filter;
	bool type_filter_solo = false;
	tools_t::rec_type_t sidebar_active_type = tools_t::rec_type_t::unknown;
};

enum class dict_kind_t
{
	user,
	base
};

struct dict_slot_t
{
	tools_t::dict_t data;
	std::string path;
	bool dirty = false;
	std::set<std::pair<tools_t::rec_type_t, size_t>> modified_records;
	filter_state_t filters;
};

class dict_workspace_t
{
public:
	int load_dict(const std::string & path, dict_kind_t kind);
	int add_slot(dict_slot_t slot, dict_kind_t kind);
	bool unload_dict(int slot_index);
	void unload_all();
	bool save_dict(int slot_index);
	bool save_dict_as(int slot_index, const std::string & path);
	void save_all_dirty();

	int find_by_path(const std::string & path) const;
	void set_active(int slot_index);
	int get_active_index() const;
	dict_slot_t * get_active_slot();
	const dict_slot_t * get_active_slot() const;

	dict_slot_t * get_slot(int index);
	const dict_slot_t * get_slot(int index) const;
	int slot_count() const;

	bool is_user_slot(int index) const;
	bool is_base_slot(int index) const;
	bool has_any_unsaved() const;

	const std::vector<dict_slot_t> & slots() const;
	const std::vector<dict_slot_t> & get_all_slots() const { return slots_; }

private:
	std::vector<dict_slot_t> slots_;
	std::vector<dict_kind_t> kinds_;
	int active_index_ = -1;
};
