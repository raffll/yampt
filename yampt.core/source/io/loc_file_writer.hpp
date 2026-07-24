#pragma once

#include "io/loc_types.hpp"

#include <string>
#include <vector>

namespace loc_file_writer
{
    void write(const std::string & path, const std::vector<loc_types::loc_entry_t> & entries);
}
