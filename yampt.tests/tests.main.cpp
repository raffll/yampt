#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <iostream>
#include <string>

std::string g_master_path;

int main(int argc, char * argv[])
{
	Catch::Session session;

	using namespace Catch::clara;
	auto cli = session.cli() |
	           Opt(g_master_path, "path")["--master-path"]("Path to master directory with ESM files (required)");

	session.cli(cli);

	int ret = session.applyCommandLine(argc, argv);
	if (ret != 0)
		return ret;

	if (g_master_path.empty())
	{
		std::cerr << "Error: --master-path is required\n";
		return 1;
	}

	if (g_master_path.back() != '/' && g_master_path.back() != '\\')
		g_master_path += '/';

	return session.run();
}
