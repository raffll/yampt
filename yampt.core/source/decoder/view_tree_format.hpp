#pragma once

#include "sub_record_schema.hpp"
#include <io/codepage.hpp>
#include <map>
#include <string>

const std::map<std::string, const char *> & sub_record_descriptions();
std::string format_value(const char * data, size_t size, codepage_t codepage = codepage_t::windows_1252);
std::string format_value_full(const char * data, size_t size, codepage_t codepage = codepage_t::windows_1252);
std::string decode_field(const field_def_t & field, const char * data, size_t data_size);
std::string make_sub_label(const std::string & sub_type, const std::string & record_type, size_t data_size);
