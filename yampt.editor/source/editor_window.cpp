#include "editor_window.hpp"
#include "dialog/editor_settings_dialog.hpp"
#include "view/plugin_workspace_view.hpp"
#include <app_settings.hpp>
#include <theme_system.hpp>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QMenuBar>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
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

	m_plugin_workspace_view = new plugin_workspace_view_t(m_settings, central);
	layout->addWidget(m_plugin_workspace_view);

	setCentralWidget(central);

	setup_menu_bar();
	setup_toolbar();
	load_config();

	connect(&theme_system_t::instance(), &theme_system_t::theme_changed,
	        this, [this](theme_t) {
		theme_system_t::instance().apply_to_application();
		m_plugin_workspace_view->refresh_views();
	});
}

void editor_window_t::setup_menu_bar()
{
	auto * file_menu = menuBar()->addMenu("&File");

	auto * load_action = new QAction("Open &Folder...", this);
	load_action->setShortcut(QKeySequence("Ctrl+O"));
	load_action->setToolTip("Load all plugins from a folder");
	file_menu->addAction(load_action);
	connect(load_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_load_data_files);

	auto * load_mo2_action = new QAction("Open &MO2 Profile...", this);
	load_mo2_action->setToolTip("Load plugins from a Mod Organizer 2 profile");
	file_menu->addAction(load_mo2_action);
	connect(
	    load_mo2_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_load_mo2_profile);

	auto * load_openmw_action = new QAction("Open Open&MW Config...", this);
	load_openmw_action->setToolTip("Load plugins from an openmw.cfg file");
	file_menu->addAction(load_openmw_action);
	connect(
	    load_openmw_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_load_openmw_cfg);

	file_menu->addSeparator();

	auto * unload_action = new QAction("&Unload All", this);
	unload_action->setToolTip("Unload all plugins and clear the list");
	file_menu->addAction(unload_action);
	connect(unload_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_unload_all);

	file_menu->addSeparator();

	auto * quit_action = new QAction("&Quit", this);
	quit_action->setShortcut(QKeySequence("Alt+F4"));
	quit_action->setToolTip("Exit the application");
	file_menu->addAction(quit_action);
	connect(quit_action, &QAction::triggered, this, &QMainWindow::close);

	auto * view_menu = menuBar()->addMenu("&View");

	auto * conflicts_action = new QAction("&Conflicts Only", this);
	conflicts_action->setCheckable(true);
	conflicts_action->setToolTip("Show only conflicting records");
	view_menu->addAction(conflicts_action);
	connect(conflicts_action, &QAction::toggled, m_plugin_workspace_view->conflicts_checkbox(), &QCheckBox::setChecked);
	connect(m_plugin_workspace_view->conflicts_checkbox(), &QCheckBox::toggled, conflicts_action, &QAction::setChecked);

	auto * hide_dup_action = new QAction("&Hide Duplicate Columns", this);
	hide_dup_action->setCheckable(true);
	hide_dup_action->setToolTip("Hide duplicate columns from the same plugin");
	view_menu->addAction(hide_dup_action);
	connect(hide_dup_action, &QAction::toggled, m_plugin_workspace_view, &plugin_workspace_view_t::set_hide_duplicates);

	view_menu->addSeparator();

	auto * filter_action = new QAction("&Filter...", this);
	filter_action->setToolTip("Open the advanced filter dialog");
	view_menu->addAction(filter_action);
	connect(filter_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_advanced_filter);

	auto * tools_menu = menuBar()->addMenu("&Tools");
	auto * settings_action = new QAction("&Preferences...", this);
	settings_action->setShortcut(QKeySequence("Ctrl+,"));
	settings_action->setToolTip("Open application settings");
	tools_menu->addAction(settings_action);
	connect(settings_action, &QAction::triggered, this, &editor_window_t::on_open_settings);
}

void editor_window_t::setup_toolbar()
{
	auto * toolbar = addToolBar("Main");
	toolbar->setMovable(false);

	auto * merge_action = new QAction("Create Merged Patch", this);
	merge_action->setToolTip("Create a merged patch from loaded plugins");
	toolbar->addAction(merge_action);
	connect(
	    merge_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_create_merged_patch);

	toolbar->addSeparator();

	statusBar()->addWidget(m_plugin_workspace_view->status_label());
	statusBar()->addPermanentWidget(m_plugin_workspace_view->count_label());
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

	m_plugin_workspace_view->restore_session_state();
}

void editor_window_t::save_config()
{
	QSettings settings(config_path, QSettings::IniFormat);
	settings.setValue("window/geometry", saveGeometry());
	settings.setValue("window/state", saveState());

	m_plugin_workspace_view->save_session_state();
}

void editor_window_t::closeEvent(QCloseEvent * event)
{
	save_config();
	event->accept();
}

void editor_window_t::on_open_settings()
{
	editor_settings_dialog_t dialog(m_settings, this);
	dialog.exec();
}
