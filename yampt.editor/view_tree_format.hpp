#pragma once

#include "../yampt/plugin_scan/sub_record_schema.hpp"
#include <map>
#include <string>

const std::map<std::string, const char *> & sub_record_descriptions();
std::string format_value(const char * data, size_t size);
std::string decode_field(const field_def_t & field, const char * data, size_t data_size);
std::string make_sub_label(const std::string & sub_type, const std::string & record_type, size_t data_size);
