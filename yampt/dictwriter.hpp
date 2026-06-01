#pragma once

#include "tools.hpp"

class DictWriter
{
public:
    static void write(const Tools::Dict & dict, const std::string & path);
};
