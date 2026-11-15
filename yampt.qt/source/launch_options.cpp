#include "launch_options.hpp"

namespace launch_options {

namespace {

constexpr const char * console_flag = "--console";
constexpr const char * debug_flag = "--debug";

} // namespace

launch_options_t parse(const std::vector<std::string> & arguments)
{
	launch_options_t options;
	for (const auto & argument : arguments)
	{
		if (argument == console_flag)
			options.console = true;

		if (argument == debug_flag)
			options.debug = true;
	}

	return options;
}

} // namespace launch_options
