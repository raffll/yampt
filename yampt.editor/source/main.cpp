#include <resource_paths.hpp>
#include "editor_window.hpp"
#include <settings_store.hpp>
#include <theme_system.hpp>
#include <utility/app_logger.hpp>
#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QPalette>
#include <QStyleFactory>
#include <QTranslator>

int main(int argc, char * argv[])
{
	app_logger_t::set_debug(true); // internal: surfaces [debug] diagnostics in the Log tab; flip to false to silence

	QApplication app(argc, argv);
	app.setStyle(QStyleFactory::create("Fusion"));
	app.setWindowIcon(QIcon(":/icons/yampt-editor.svg"));
	QTranslator translator;
	const auto ui_languages = QLocale::system().uiLanguages();
	for (const auto & locale : ui_languages)
	{
		if (translator.load(
		        "yEditor_" + QLocale(locale).name(), QString::fromStdString(resource_paths::translations_dir())))
		{
			app.installTranslator(&translator);
			break;
		}
	}

	settings_store_t startup_settings("yEditor.ini");
	theme_system_t::instance().set_theme(startup_settings.theme());
	theme_system_t::instance().apply_to_application();

	editor_window_t window;
	window.showMaximized();

	return app.exec();
}
