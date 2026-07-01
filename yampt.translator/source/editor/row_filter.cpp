#include "row_filter.hpp"
#include <utility/string_utils.hpp>
#include <algorithm>

void row_filter_t::set_config(const config_t & cfg)
{
	m_config = cfg;
	m_compiled_regex = std::nullopt;

	if (!m_config.regex_mode || m_config.query.empty())
		return;

	try
	{
		auto flags = std::regex_constants::ECMAScript;
		if (!m_config.case_sensitive)
			flags |= std::regex_constants::icase;

		m_compiled_regex = std::regex(m_config.query, flags);
	}
	catch (const std::regex_error &)
	{}
}

bool row_filter_t::has_query() const
{
	return !m_config.query.empty();
}

bool row_filter_t::matches(const table_row_t & row) const
{
	if (m_config.query.empty())
		return true;

	auto test_column = [&](search_column_t col) -> const std::string *
	{
		switch (col)
		{
		case search_column_t::key:
			return &row.key_text;
		case search_column_t::original:
			return &row.old_text;
		case search_column_t::translation:
			return &row.new_text;
		}
		return nullptr;
	};

	if (m_config.regex_mode)
	{
		if (!m_compiled_regex)
			return false;

		for (const auto & col : m_config.columns)
		{
			const auto * text = test_column(col);
			if (!text)
				continue;

			try
			{
				if (std::regex_search(*text, *m_compiled_regex))
					return true;
			}
			catch (const std::regex_error &)
			{}
		}
		return false;
	}

	const auto query = m_config.case_sensitive ? m_config.query : string_utils::to_lower(m_config.query);

	for (const auto & col : m_config.columns)
	{
		const auto * text = test_column(col);
		if (!text)
			continue;

		const auto haystack = m_config.case_sensitive ? *text : string_utils::to_lower(*text);
		if (haystack.find(query) != std::string::npos)
			return true;
	}

	return false;
}
