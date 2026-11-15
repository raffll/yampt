#pragma once

#include <string>
#include <vector>

namespace launch_options {

struct launch_options_t
{
	bool console = false;
	bool debug = false;
};

launch_options_t parse(const std::vector<std::string> & arguments);

} // namespace launch_options
