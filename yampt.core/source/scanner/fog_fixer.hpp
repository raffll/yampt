#pragma once

#include <string>

class fog_fixer_t
{
public:
	static std::string apply(const std::string & content);

private:
	static bool is_interior_cell(const std::string & content);
	static bool has_behave_exterior_flag(const std::string & content);
};
