#pragma once

#include "../utility/domain_types.hpp"

#include <string>
#include <vector>

class binary_file_io_t
{
public:
	static std::string read_file(const std::string & path);
	static void write_text(const std::string & text, const std::string & path);
	static void write_file(const std::vector<record_t> & records, const std::string & path);
	static void create_file(const std::vector<record_t> & records, const std::string & path);
};
