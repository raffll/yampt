#pragma once

#include "io/codepage.hpp"
#include "io/loc_types.hpp"

#include <string>
#include <vector>

namespace loc_file_reader
{
    struct loc_file_t
    {
        loc_types::loc_file_kind_t file_kind;
        std::vector<loc_types::loc_entry_t> entries;
    };

    loc_file_t read(const std::string & path, codepage_t codepage);
    loc_types::loc_file_kind_t classify_extension(const std::string & path);
}
