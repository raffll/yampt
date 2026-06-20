#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

#include <iostream>
#include <string>

std::string g_master_path;

int main(int argc, char * argv[])
{
	Catch::Session session;

	auto cli = session.cli() |
	           Catch::Clara::Opt(g_master_path, "path")["--master-path"]("Path to master directory with ESM files");

	session.cli(cli);

	int ret = session.applyCommandLine(argc, argv);
	if (ret != 0)
		return ret;

	if (g_master_path.empty())
		g_master_path = "C:/OMEN/Morrowind/master/";

	if (g_master_path.back() != '/' && g_master_path.back() != '\\')
		g_master_path += '/';

	return session.run();
}
