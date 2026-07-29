#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>
#include <utility/app_logger.hpp>
#include <QCoreApplication>

int main(int argc, char * argv[])
{
	QCoreApplication app(argc, argv);

	Catch::Session session;

	int ret = session.applyCommandLine(argc, argv);
	if (ret != 0)
		return ret;

	app_logger_t::set_quiet(false);

	return session.run();
}
