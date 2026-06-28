#pragma once

#include "table_row.hpp"
#include "../controller/search_engine.hpp"
#include "../../yampt/utility/tools.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

struct dict_counts_t
{
	std::map<tools_t::rec_type_t, size_t> type_counts;
	std::map<tools_t::rec_type_t, size_t> translated_counts;
	std::map<std::string, size_t> total_status_counts;
	std::map<std::string, size_t> filtered_status_counts;
	std::map<std::string, size_t> sub_type_total_counts;
	std::map<std::string, size_t> sub_type_translated_counts;
	size_t progress_total = 0;
	size_t progress_translated = 0;
};

struct table_build_result_t
{
	std::vector<table_row_t> rows;
	dict_counts_t counts;
};

struct table_filter_params_t
{
	const std::set<tools_t::rec_type_t> & type_filter;
	const std::set<std::string> & sub_type_filter;
	const std::set<std::string> & status_filter;
	const search_engine_t & search;
	bool type_filter_solo;
};

table_build_result_t build_filtered_rows(const tools_t::dict_t & data, const table_filter_params_t & params);
