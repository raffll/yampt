#pragma once

#include <scanner/plugin_scan.hpp>
#include <set>
#include <string>

class nav_tree_filter_t
{
public:
	struct filter_state_t
	{
		bool filter_conflict_all = false;
		std::set<conflict_all_t> conflict_all_set;
		bool filter_conflict_this = false;
		std::set<conflict_this_t> conflict_this_set;
		bool filter_by_type = false;
		std::set<std::string> type_set;
		bool filter_by_id = false;
		std::string id_text;
		bool filter_by_name = false;
		std::string name_text;
		bool filter_deleted = false;

		bool operator==(const filter_state_t &) const = default;
	};

	void set_filter(const filter_state_t & state);
	void clear();
	void set_hide_duplicates(bool hide);
	void set_excluded_plugins(const std::set<std::string> * excluded);
	void set_patch_plugins(const std::set<std::string> * patch);

	bool passes(const conflict_entry_t & entry, int plugin_idx) const;
	bool has_active_filter() const;
	bool hide_duplicates() const;
	const std::set<std::string> * excluded_plugins() const;
	const std::set<std::string> * patch_plugins() const;

private:
	filter_state_t m_filter;
	bool m_has_filter = false;
	bool m_hide_duplicates = false;
	const std::set<std::string> * m_excluded_plugins = nullptr;
	const std::set<std::string> * m_patch_plugins = nullptr;

	static bool contains_case_insensitive(const std::string & haystack, const std::string & needle);
	static bool has_version_status(const conflict_entry_t & entry, int plugin_idx, conflict_this_t status);
};
