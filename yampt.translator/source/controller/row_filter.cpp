#include "row_filter.hpp"
#include <utility/string_utils.hpp>
#include <algorithm>

void row_filter_t::set_config(const config_t & cfg)
{
	config_ = cfg;
	compiled_regex_ = std::nullopt;

	if (!config_.regex_mode || config_.query.empty())
		return;

	try
	{
		auto flags = std::regex_constants::ECMAScript;
		if (!config_.case_sensitive)
			flags |= std::regex_constants::icase;

		compiled_regex_ = std::regex(config_.query, flags);
	}
	catch (const std::regex_error &)
	{}
}

bool row_filter_t::has_query() const
{
	return !config_.query.empty();
}

bool row_filter_t::matches(const table_row_t & row) const
{
	if (config_.query.empty())
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

	if (config_.regex_mode)
	{
		if (!compiled_regex_)
			return false;

		for (const auto & col : config_.columns)
		{
			const auto * text = test_column(col);
			if (!text)
				continue;

			try
			{
				if (std::regex_search(*text, *compiled_regex_))
					return true;
			}
			catch (const std::regex_error &)
			{}
		}
		return false;
	}

	const auto query = config_.case_sensitive ? config_.query : string_utils::to_lower(config_.query);

	for (const auto & col : config_.columns)
	{
		const auto * text = test_column(col);
		if (!text)
			continue;

		const auto haystack = config_.case_sensitive ? *text : string_utils::to_lower(*text);
		if (haystack.find(query) != std::string::npos)
			return true;
	}

	return false;
}
