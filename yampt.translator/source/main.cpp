#include <resource_paths.hpp>
#include "main_window.hpp"
#include <utility/app_logger.hpp>
#include <settings_store.hpp>
#include <theme_system.hpp>
#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QPalette>
#include <QStyleFactory>
#include <QTranslator>

int main(int argc, char * argv[])
{
	QApplication app(argc, argv);
	app.setStyle(QStyleFactory::create("Fusion"));
	app.setWindowIcon(QIcon(":/icons/yampt-translator.svg"));

	QTranslator translator;
	const auto ui_languages = QLocale::system().uiLanguages();
	for (const auto & locale : ui_languages)
	{
		if (translator.load(
		        "yTranslator_" + QLocale(locale).name(), QString::fromStdString(resource_paths::translations_dir())))
		{
			app.installTranslator(&translator);
			break;
		}
	}

	settings_store_t startup_settings("yTranslator.ini");
	theme_system_t::instance().set_theme(startup_settings.theme());
	theme_system_t::instance().apply_to_application();

	main_window_t window;
	window.showMaximized();

	return app.exec();
}
