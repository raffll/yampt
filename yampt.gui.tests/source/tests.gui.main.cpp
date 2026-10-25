#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>
#include <utility/app_logger.hpp>
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QString>

namespace {

QByteArray qt_plugins_dir()
{
	const auto solution_dir = QString(QStringLiteral(YAMPT_SOLUTION_DIR));

#ifdef _DEBUG
	const auto relative_plugins =
	    QStringLiteral("vcpkg_installed/x64-windows/x64-windows/debug/Qt6/plugins");
#else
	const auto relative_plugins =
	    QStringLiteral("vcpkg_installed/x64-windows/x64-windows/Qt6/plugins");
#endif

	const auto joined = QDir(solution_dir).filePath(relative_plugins);
	return QDir::cleanPath(joined).toLocal8Bit();
}

} // namespace

int main(int argc, char * argv[])
{
	qputenv("QT_QPA_PLATFORM", "offscreen");
	qputenv("QT_PLUGIN_PATH", qt_plugins_dir());

	QApplication app(argc, argv);

	Catch::Session session;

	const int command_line_result = session.applyCommandLine(argc, argv);
	if (command_line_result != 0)
		return command_line_result;

	app_logger_t::set_quiet(false);
	app_logger_t::add_log("[info] gui test main: offscreen qapplication constructed");

	return session.run();
}
