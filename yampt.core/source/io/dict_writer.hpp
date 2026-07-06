#pragma once

#include "../utility/domain_types.hpp"

class dict_writer_t
{
public:
	static void write(const dict_t & dict, const std::string & path);
};
