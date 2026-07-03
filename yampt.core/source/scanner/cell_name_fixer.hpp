#pragma once

#include <string>
#include <vector>

class cell_name_fixer_t
{
public:
	static std::string apply(const std::vector<std::string> & version_contents);

private:
	static bool is_exterior_cell(const std::string & content);
	static std::string get_cell_name(const std::string & content);
	static std::string set_cell_name(const std::string & content, const std::string & name);
	static std::string find_last_intermediate_rename(
	    const std::vector<std::string> & version_contents,
	    const std::string & first_name);
};
