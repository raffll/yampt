#pragma once

#include <string>

#include "../yampt/tools.hpp"

enum class validation_level_t
{
	ok,
	caution,
	error
};

struct validation_result_t
{
	validation_level_t level;
	size_t byte_count;
	size_t limit;
};

class validation_manager_t
{
public:
	validation_result_t validate(tools_t::rec_type_t type, const std::string & value) const;
};
