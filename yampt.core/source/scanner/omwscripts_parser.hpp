#pragma once

#include <string>
#include <vector>

struct omwscripts_entry_t
{
	std::vector<std::string> context_tags;
	std::string script_path;
	int line_number;
};

struct omwscripts_result_t
{
	std::string source_file;
	std::string mod_name;
	std::vector<omwscripts_entry_t> entries;
	std::vector<std::string> warnings;
};

class omwscripts_parser_t
{
public:
	omwscripts_result_t parse(const std::string & file_path,
	                          const std::string & mod_name);
};
