#pragma once

#include "tools.hpp"

class DictWriter
{
public:
    static void write(const tools_t::dict_t & dict, const std::string & path);
};
