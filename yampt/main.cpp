#include "includes.hpp"
#include "tools.hpp"
#include "user_interface.hpp"

int main(int argc, char * argv[])
{
	try
	{
		std::vector<std::string> arg;
		for (int i = 0; i < argc; i++)
		{
			arg.push_back(argv[i]);
		}
		user_interface_t ui(arg);
	}
	catch (const std::exception & e)
	{
		tools_t::add_log("[error] " + std::string(e.what()) + "\r\n");
	}
	catch (...)
	{
		tools_t::add_log("[error] unknown error\r\n");
	}

	tools_t::write_text(tools_t::get_log(), "yampt.log");
	return tools_t::has_error() ? 1 : 0;
}
