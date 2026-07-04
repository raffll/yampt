#pragma once

#include <string>
#include <vector>

struct view_group_def_t
{
	const char * record_type;
	const char * group_label;
	const char * sub_types[8];
};

const std::vector<view_group_def_t> & view_group_definitions();

const char * find_group_label(const std::string & record_type, const std::string & sub_type);
