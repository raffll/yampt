#pragma once

#include <utility/tools.hpp>
#include <set>
#include <string>

struct filter_state_t
{
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;
	bool type_filter_solo = false;
};
