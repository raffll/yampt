#pragma once

#include "record_table_model.hpp"
#include <optional>
#include <regex>
#include <set>
#include <string>

enum class search_column_t
{
	key,
	original,
	translation
};

class search_engine_t
{
public:
	struct config_t
	{
		std::string query;
		bool case_sensitive = false;
		bool regex_mode = false;
		std::set<search_column_t> columns = { search_column_t::key,
			                                  search_column_t::original,
			                                  search_column_t::translation };
	};

	void set_config(const config_t & cfg);
	bool matches(const table_row_t & row) const;
	bool has_query() const;

private:
	config_t config_;
	std::optional<std::regex> compiled_regex_;
};
