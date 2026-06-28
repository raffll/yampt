#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

#include "../yampt/utility/tools.hpp"

int main(int argc, char * argv[])
{
	Catch::Session session;

	int ret = session.applyCommandLine(argc, argv);
	if (ret != 0)
		return ret;

	tools_t::set_quiet(false);

	return session.run();
}
