#pragma once

#include "../utility/domain_types.hpp"

#include <string>
#include <vector>

namespace binary_file_io {

std::string read_file(const std::string & path);
void write_text(const std::string & text, const std::string & path);
void write_file(const std::vector<record_t> & records, const std::string & path);
void create_file(const std::vector<record_t> & records, const std::string & path);

} // namespace binary_file_io
