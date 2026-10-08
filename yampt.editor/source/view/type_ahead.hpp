#pragma once

#include <string>
#include <vector>

namespace type_ahead {

int next_matching_row(const std::vector<std::string> & filenames, const std::string & lowered_prefix, int start_row);

}
