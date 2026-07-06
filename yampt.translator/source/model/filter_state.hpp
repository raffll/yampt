#pragma once

#include <utility/domain_types.hpp>
#include <set>
#include <string>

struct filter_state_t
{
	std::set<rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;
	bool type_filter_solo = false;
};
