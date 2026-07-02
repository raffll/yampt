#include "main_window.hpp"
#include <io/app_settings.hpp>
#include <theme_system.hpp>
#include <utility/tools.hpp>
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char * argv[])
{
	QApplication app(argc, argv);
	app.setStyle(QStyleFactory::create("Fusion"));

	tools_t::set_exe_dir(QCoreApplication::applicationDirPath().toStdString());

	app_settings_t startup_settings("yTranslator.ini");
	theme_system_t::instance().set_theme(startup_settings.theme());
	theme_system_t::instance().apply_to_application();

	main_window_t window;
	window.show();

	return app.exec();
}
