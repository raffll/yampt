#pragma once

#include "../io/esm_reader.hpp"
#include <set>
#include <string>

namespace info_place_values {

void collect_from_info_record(const std::string & record_content, std::set<std::string> & place_values);

std::set<std::string> collect_from_plugin(const esm_reader_t & plugin);

} // namespace info_place_values
