#include "nav_tree_filter.hpp"
#include <utility/string_utils.hpp>
#include <algorithm>
#include <cctype>
#include <regex>

bool nav_tree_filter_t::contains_case_insensitive(const std::string & haystack, const std::string & needle)
{
	if (needle.size() > haystack.size())
		return false;

	const auto haystack_lower = string_utils::to_lower_utf8(haystack);
	const auto needle_lower = string_utils::to_lower_utf8(needle);

	return haystack_lower.find(needle_lower) != std::string::npos;
}

bool nav_tree_filter_t::matches_text(const std::string & haystack, const std::string & needle) const
{
	if (m_filter.search_regex)
	{
		try
		{
			auto flags = std::regex_constants::ECMAScript;
			if (!m_filter.search_case_sensitive)
				flags |= std::regex_constants::icase;

			std::regex pattern(needle, flags);
			return std::regex_search(haystack, pattern);
		}
		catch (const std::regex_error &)
		{
			return false;
		}
	}

	if (m_filter.search_case_sensitive)
		return haystack.find(needle) != std::string::npos;

	return contains_case_insensitive(haystack, needle);
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

void nav_tree_filter_t::set_hide_duplicates(bool hide)
{
	m_hide_duplicates = hide;
}

void nav_tree_filter_t::set_excluded_plugins(const std::set<std::string> * excluded)
{
	m_excluded_plugins = excluded;
}

void nav_tree_filter_t::set_patch_plugins(const std::set<std::string> * patch)
{
	m_patch_plugins = patch;
}

void nav_tree_filter_t::set_dirty_plugins(const std::set<std::string> * dirty)
{
	m_dirty_plugins = dirty;
}

bool nav_tree_filter_t::has_active_filter() const
{
	return m_has_filter;
}

bool nav_tree_filter_t::hide_duplicates() const
{
	return m_hide_duplicates;
}

const std::set<std::string> * nav_tree_filter_t::excluded_plugins() const
{
	return m_excluded_plugins;
}

const std::set<std::string> * nav_tree_filter_t::patch_plugins() const
{
	return m_patch_plugins;
}

const std::set<std::string> * nav_tree_filter_t::dirty_plugins() const
{
	return m_dirty_plugins;
}

bool nav_tree_filter_t::has_version_status(const conflict_entry_t & entry, int plugin_idx, conflict_this_t status)
{
	for (const auto & version : entry.versions)
	{
		if (version.plugin_idx == plugin_idx && version.status == status)
			return true;
	}
	return false;
}

bool nav_tree_filter_t::passes_lua_conflict(const handler_conflict_t & conflict) const
{
	if (m_filter.filter_lua_severity && !m_filter.lua_severity_set.empty())
	{
		if (m_filter.lua_severity_set.find(conflict.severity) == m_filter.lua_severity_set.end())
			return false;
	}

	if (m_filter.filter_lua_interface && !m_filter.lua_interface_set.empty())
	{
		if (m_filter.lua_interface_set.find(conflict.interface_name) == m_filter.lua_interface_set.end())
			return false;
	}

	return true;
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
		if (!matches_text(entry.record_id, m_filter.id_text))
			return false;
	}

	if (m_filter.filter_by_name && !m_filter.name_text.empty())
	{
		if (!matches_text(entry.display_name, m_filter.name_text))
			return false;
	}

	if (m_filter.filter_deleted)
	{
		if (!entry.has_dele)
			return false;
	}

	return true;
}
