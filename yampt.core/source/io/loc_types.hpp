#pragma once

#include <string>
#include <vector>

namespace loc_types
{
    struct loc_entry_t
    {
        std::string key;
        std::string value;
    };

    enum class loc_file_kind_t
    {
        cel,
        top,
        mrk
    };
}
