#include "editor_window.hpp"
#include "editor_tab.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QMenuBar>
#include <QSettings>
#include <QVBoxLayout>

static const QString config_path = "yEditor.ini";

editor_window_t::editor_window_t(QWidget * parent)
    : QMainWindow(parent)
{
	setWindowTitle("yEditor");
	resize(1400, 900);

	auto * central = new QWidget(this);
	auto * layout = new QVBoxLayout(central);
	layout->setContentsMargins(0, 0, 0, 0);

	editor_tab_ = new editor_tab_t(central);
	layout->addWidget(editor_tab_);

	setCentralWidget(central);

	setup_menu_bar();
	load_config();
}

void editor_window_t::setup_menu_bar()
{
	auto * file_menu = menuBar()->addMenu("&File");

	auto * load_action = new QAction("Open &Folder...", this);
	load_action->setShortcut(QKeySequence("Ctrl+O"));
	load_action->setToolTip("Load all plugins from a folder");
	file_menu->addAction(load_action);
	connect(load_action, &QAction::triggered, editor_tab_, &editor_tab_t::on_load_data_files);

	auto * load_mo2_action = new QAction("Open &MO2 Profile...", this);
	load_mo2_action->setToolTip("Load plugins from a Mod Organizer 2 profile");
	file_menu->addAction(load_mo2_action);
	connect(load_mo2_action, &QAction::triggered, editor_tab_, &editor_tab_t::on_load_mo2_profile);

	auto * load_openmw_action = new QAction("Open Open&MW Config...", this);
	load_openmw_action->setToolTip("Load plugins from an openmw.cfg file");
	file_menu->addAction(load_openmw_action);
	connect(load_openmw_action, &QAction::triggered, editor_tab_, &editor_tab_t::on_load_openmw_cfg);

	file_menu->addSeparator();

	auto * save_action = new QAction("&Save Plugin", this);
	save_action->setShortcut(QKeySequence("Ctrl+S"));
	save_action->setToolTip("Save the active plugin to disk");
	save_action->setVisible(false);
	file_menu->addAction(save_action);
	connect(save_action, &QAction::triggered, editor_tab_, &editor_tab_t::on_save_plugin);

	file_menu->addSeparator();

	auto * quit_action = new QAction("&Quit", this);
	quit_action->setShortcut(QKeySequence("Alt+F4"));
	quit_action->setToolTip("Exit the application");
	file_menu->addAction(quit_action);
	connect(quit_action, &QAction::triggered, this, &QMainWindow::close);
}

void editor_window_t::load_config()
{
	QSettings settings(config_path, QSettings::IniFormat);
	auto geom = settings.value("window/geometry").toByteArray();
	if (!geom.isEmpty())
		restoreGeometry(geom);

	auto state = settings.value("window/state").toByteArray();
	if (!state.isEmpty())
		restoreState(state);
}

void editor_window_t::save_config()
{
	QSettings settings(config_path, QSettings::IniFormat);
	settings.setValue("window/geometry", saveGeometry());
	settings.setValue("window/state", saveState());
}

void editor_window_t::closeEvent(QCloseEvent * event)
{
	save_config();
	event->accept();
}
