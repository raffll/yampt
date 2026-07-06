#include "main_window.hpp"
#include <utility/app_logger.hpp>
#include <settings_store.hpp>
#include <theme_system.hpp>
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char * argv[])
{
	QApplication app(argc, argv);
	app.setStyle(QStyleFactory::create("Fusion"));

	app_logger_t::set_exe_dir(QCoreApplication::applicationDirPath().toStdString());

	settings_store_t startup_settings("yTranslator.ini");
	theme_system_t::instance().set_theme(startup_settings.theme());
	theme_system_t::instance().apply_to_application();

	main_window_t window;
	window.show();

	return app.exec();
}
