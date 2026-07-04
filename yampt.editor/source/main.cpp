#include "editor_window.hpp"
#include <settings_store.hpp>
#include <theme_system.hpp>
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char * argv[])
{
	QApplication app(argc, argv);
	app.setStyle(QStyleFactory::create("Fusion"));

	settings_store_t startup_settings("yEditor.ini");
	theme_system_t::instance().set_theme(startup_settings.theme());
	theme_system_t::instance().apply_to_application();

	editor_window_t window;
	window.show();

	return app.exec();
}
