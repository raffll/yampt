#pragma once

#include "../utility/domain_types.hpp"
#include <set>
#include <string>

struct eet_build_input_t
{
	const dict_t & native_dict;
	const dict_t & eet_pool;
	const std::set<std::string> & referenced_places;
};

namespace eet_dict_builder
{
dict_t build(const eet_build_input_t & input);
}
