#pragma once

#include "glossary.hpp"
#include <io/loc_types.hpp>
#include <vector>

namespace loc_inflection_annotations {

std::vector<annotation_t> build(
    loc_types::loc_file_kind_t file_kind,
    const std::vector<loc_types::loc_entry_t> & entries);

} // namespace loc_inflection_annotations
