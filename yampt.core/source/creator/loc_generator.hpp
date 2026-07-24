#pragma once

#include "io/codepage.hpp"
#include "utility/domain_types.hpp"

#include <string>

namespace loc_generator
{
    struct generation_input_t
    {
        const dict_t & dict;
        std::string output_directory;
        std::string esm_name;
        codepage_t codepage;
        std::string hunspell_aff_path;
        std::string hunspell_dic_path;
    };

    struct generation_result_t
    {
        int cel_entries = 0;
        int mrk_entries = 0;
        int top_entries = 0;
        int skipped_entries = 0;
        int collision_warnings = 0;
        std::string cel_path;
        std::string mrk_path;
        std::string top_path;
    };

    generation_result_t generate(const generation_input_t & input);

    std::string derive_esm_name(const std::string & filename);
}
