#pragma once

#include <utility/domain_types.hpp>
#include <string>

enum table_col_t
{
	col_id = 0,
	col_key = 1,
	col_original = 2,
	col_translation = 3,
	col_status = 4,
	col_count = 5
};

struct table_row_t
{
	rec_type_t type;
	std::string key_text;
	bool is_child = false;
	std::string old_text;
	std::string new_text;
	status_t status = status_t::untranslated;
	size_t record_index;
};
