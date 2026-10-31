#pragma once

#include "../utility/domain_types.hpp"

class eet_dict_builder_t
{
public:
	static dict_t build(const dict_t & native_dict, const dict_t & eet_pool);
};
