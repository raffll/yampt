#pragma once

#include "tools.hpp"

class dict_writer_t
{
public:
	static void write(
	    const tools_t::dict_t & dict,
	    const std::string & path,
	    tools_t::encoding_t encoding = tools_t::encoding_t::unknown);
};
