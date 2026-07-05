#include "nav_tree_filter.hpp"
#include <algorithm>
#include <cctype>

bool nav_tree_filter_t::contains_case_insensitive(const std::string & haystack, const std::string & needle)
{
	if (needle.size() > haystack.size())
		return false;

	return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
	    [](char a, char b)
	    { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })
	    != haystack.end();
}

void nav_tree_filter_t::set_filter(const filter_state_t & state)
{
	m_has_filter = true;
	m_filter = state;
}

void nav_tree_filter_t::clear()
{
	m_has_filter = false;
	m_filter = {};
}

void nav_tree_filter_t::set_hide_duplicates(bool hide) { m_hide_duplicates = hide; }

void nav_tree_filter_t::set_excluded_plugins(const std::set<std::string> * excluded) { m_excluded_plugins = excluded; }

void nav_tree_filter_t::set_patch_plugins(const std::set<std::string> * patch) { m_patch_plugins = patch; }

bool nav_tree_filter_t::has_active_filter() const { return m_has_filter; }

bool nav_tree_filter_t::hide_duplicates() const { return m_hide_duplicates; }

const std::set<std::string> * nav_tree_filter_t::excluded_plugins() const { return m_excluded_plugins; }

const std::set<std::string> * nav_tree_filter_t::patch_plugins() const { return m_patch_plugins; }

bool nav_tree_filter_t::has_version_status(const conflict_entry_t & entry, int plugin_idx, conflict_this_t status)
{
	for (const auto & version : entry.versions)
	{
		if (version.plugin_idx == plugin_idx && version.status == status)
			return true;
	}
	return false;
}

bool nav_tree_filter_t::passes(const conflict_entry_t & entry, int plugin_idx) const
{
	if (!m_has_filter)
		return true;

	if (m_filter.filter_conflict_all && !m_filter.conflict_all_set.empty())
	{
		if (m_filter.conflict_all_set.find(entry.conflict_all) == m_filter.conflict_all_set.end())
			return false;
	}

	if (m_filter.filter_conflict_this && !m_filter.conflict_this_set.empty())
	{
		bool found = false;
		for (const auto & version : entry.versions)
		{
			if (version.plugin_idx == plugin_idx &&
			    m_filter.conflict_this_set.find(version.status) != m_filter.conflict_this_set.end())
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	if (m_filter.filter_by_type && !m_filter.type_set.empty())
	{
		if (m_filter.type_set.find(entry.rec_type) == m_filter.type_set.end())
			return false;
	}

	if (m_filter.filter_by_id && !m_filter.id_text.empty())
	{
		if (!contains_case_insensitive(entry.record_id, m_filter.id_text))
			return false;
	}

	if (m_filter.filter_by_name && !m_filter.name_text.empty())
	{
		if (!contains_case_insensitive(entry.display_name, m_filter.name_text))
			return false;
	}

	if (m_filter.filter_deleted)
	{
		if (!entry.has_dele)
			return false;
	}

	return true;
}
