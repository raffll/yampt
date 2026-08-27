#include "editor_window.hpp"
#include "dialog/settings/editor_settings_dialog.hpp"
#include "view/plugin_workspace_view.hpp"
#include <settings_store.hpp>
#include <theme_system.hpp>
#include <QAction>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

static const QString config_path = "yEditor.ini";

editor_window_t::editor_window_t(QWidget * parent)
    : QMainWindow(parent)
{
	setWindowTitle(tr("yEditor"));
	resize(1400, 900);

	auto * central = new QWidget(this);
	auto * layout = new QVBoxLayout(central);
	layout->setContentsMargins(0, 0, 0, 0);

	m_plugin_workspace_view = new plugin_workspace_view_t(m_settings, central);
	layout->addWidget(m_plugin_workspace_view);

	setCentralWidget(central);

	load_config();
	setup_menu_bar();
	setup_toolbar();

	connect(
	    &theme_system_t::instance(),
	    &theme_system_t::theme_changed,
	    this,
	    [this](theme_t)
	{
		theme_system_t::instance().apply_to_application();
		m_plugin_workspace_view->refresh_views();
	});
}

void editor_window_t::setup_menu_bar()
{
	auto * file_menu = menuBar()->addMenu(tr("&File"));

	auto * load_action = new QAction(tr("Open &Folder..."), this);
	load_action->setShortcut(QKeySequence("Ctrl+O"));
	load_action->setToolTip(tr("Load all plugins from a folder"));
	file_menu->addAction(load_action);
	connect(load_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_load_data_files);

	auto * load_mo2_action = new QAction(tr("Open &MO2 Profile..."), this);
	load_mo2_action->setToolTip(tr("Load plugins from a Mod Organizer 2 profile"));
	file_menu->addAction(load_mo2_action);
	connect(
	    load_mo2_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_load_mo2_profile);

	auto * load_openmw_action = new QAction(tr("Open Open&MW Config..."), this);
	load_openmw_action->setToolTip(tr("Load plugins from an openmw.cfg file"));
	file_menu->addAction(load_openmw_action);
	connect(
	    load_openmw_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_load_openmw_cfg);

	file_menu->addSeparator();

	auto * unload_action = new QAction(tr("&Unload All"), this);
	unload_action->setToolTip(tr("Unload all plugins and clear the list"));
	file_menu->addAction(unload_action);
	connect(unload_action, &QAction::triggered, m_plugin_workspace_view, &plugin_workspace_view_t::on_unload_all);

	file_menu->addSeparator();

	auto * quit_action = new QAction(tr("&Quit"), this);
	quit_action->setShortcut(QKeySequence("Alt+F4"));
	quit_action->setToolTip(tr("Exit the application"));
	file_menu->addAction(quit_action);
	connect(quit_action, &QAction::triggered, this, &QMainWindow::close);

	auto * view_menu = menuBar()->addMenu(tr("&View"));

	m_conflicts_action = new QAction(tr("Conflicts Only"), this);
	m_conflicts_action->setCheckable(true);
	m_conflicts_action->setChecked(m_plugin_workspace_view->is_conflicts_only());
	m_conflicts_action->setToolTip(tr("Show only conflicting records"));
	connect(
	    m_conflicts_action,
	    &QAction::toggled,
	    m_plugin_workspace_view,
	    &plugin_workspace_view_t::set_conflicts_only);

	auto * hide_dup_action = new QAction(tr("&Hide Duplicates"), this);
	hide_dup_action->setCheckable(true);
	hide_dup_action->setChecked(m_plugin_workspace_view->is_hide_duplicates());
	hide_dup_action->setToolTip(tr("Hide duplicate columns from the same plugin"));
	view_menu->addAction(hide_dup_action);
	connect(hide_dup_action, &QAction::toggled, m_plugin_workspace_view, &plugin_workspace_view_t::set_hide_duplicates);

	auto * show_deleted_action = new QAction(tr("&Mark Deleted"), this);
	show_deleted_action->setCheckable(true);
	show_deleted_action->setChecked(m_plugin_workspace_view->is_show_deleted_strikeout());
	show_deleted_action->setToolTip(tr("Strikeout deleted records and cell references"));
	view_menu->addAction(show_deleted_action);
	connect(
	    show_deleted_action,
	    &QAction::toggled,
	    m_plugin_workspace_view,
	    &plugin_workspace_view_t::set_show_deleted_strikeout);

	view_menu->addSeparator();

	auto * tools_menu = menuBar()->addMenu(tr("&Tools"));
	auto * settings_action = new QAction(tr("&Preferences..."), this);
	settings_action->setShortcut(QKeySequence("Ctrl+,"));
	settings_action->setToolTip(tr("Open application settings"));
	tools_menu->addAction(settings_action);
	connect(settings_action, &QAction::triggered, this, &editor_window_t::on_open_settings);
}

void editor_window_t::setup_toolbar()
{
	auto * toolbar = addToolBar("Main");
	toolbar->setMovable(false);

	setContextMenuPolicy(Qt::PreventContextMenu);

	auto * merge_btn = new QToolButton(this);
	merge_btn->setText(tr("Create Merged Patch"));
	merge_btn->setToolTip(tr("Create a merged patch from loaded plugins"));
	toolbar->addWidget(merge_btn);
	connect(merge_btn, &QToolButton::clicked, m_plugin_workspace_view, &plugin_workspace_view_t::on_create_merged_patch);

	auto * clean_btn = new QToolButton(this);
	clean_btn->setText(tr("Clean All"));
	clean_btn->setToolTip(tr("Remove evil GMSTs and junk cells from all plugins"));
	toolbar->addWidget(clean_btn);
	connect(clean_btn, &QToolButton::clicked, m_plugin_workspace_view, &plugin_workspace_view_t::on_clean_all);

	toolbar->addSeparator();

	m_editing_btn = new QToolButton(this);
	m_editing_btn->setText(tr("Enable Editing"));
	m_editing_btn->setToolTip(tr("Allow editing decoded fields directly in all plugins"));
	m_editing_btn->setCheckable(true);
	m_editing_btn->setChecked(m_settings.editing_enabled());
	toolbar->addWidget(m_editing_btn);
	connect(
	    m_editing_btn,
	    &QToolButton::toggled,
	    this,
	    [this](bool checked)
	{
		m_settings.set_editing_enabled(checked);
		m_settings.sync();
		m_plugin_workspace_view->set_editing_enabled(checked);
	});

	toolbar->addSeparator();

	auto * conflicts_btn = new QToolButton(this);
	conflicts_btn->setDefaultAction(m_conflicts_action);
	toolbar->addWidget(conflicts_btn);

	toolbar->addSeparator();

	m_search_field = new QLineEdit(this);
	m_search_field->setPlaceholderText(tr("Filter by..."));
	m_search_field->setToolTip(tr("Search by record ID or display name"));
	toolbar->addWidget(m_search_field);

	m_case_sensitive_btn = new QToolButton(this);
	m_case_sensitive_btn->setText(tr("Aa"));
	m_case_sensitive_btn->setCheckable(true);
	m_case_sensitive_btn->setToolTip(tr("Case-sensitive search"));
	toolbar->addWidget(m_case_sensitive_btn);

	m_regex_btn = new QToolButton(this);
	m_regex_btn->setText(tr(".*"));
	m_regex_btn->setCheckable(true);
	m_regex_btn->setToolTip(tr("Regular expression search"));
	toolbar->addWidget(m_regex_btn);

	m_search_id_btn = new QToolButton(this);
	m_search_id_btn->setText(tr("ID"));
	m_search_id_btn->setCheckable(true);
	m_search_id_btn->setChecked(true);
	m_search_id_btn->setToolTip(tr("Search in record ID"));
	toolbar->addWidget(m_search_id_btn);

	m_search_name_btn = new QToolButton(this);
	m_search_name_btn->setText(tr("Name"));
	m_search_name_btn->setCheckable(true);
	m_search_name_btn->setChecked(true);
	m_search_name_btn->setToolTip(tr("Search in display name"));
	toolbar->addWidget(m_search_name_btn);

	toolbar->addSeparator();

	auto * filter_btn = new QToolButton(this);
	filter_btn->setText(tr("Advanced Filters..."));
	filter_btn->setToolTip(tr("Open the advanced filter dialog"));
	toolbar->addWidget(filter_btn);

	connect(m_search_field, &QLineEdit::returnPressed, this, &editor_window_t::on_search_apply);
	connect(filter_btn, &QToolButton::clicked, m_plugin_workspace_view, &plugin_workspace_view_t::on_advanced_filter);

	auto * escape_shortcut = new QShortcut(QKeySequence("Escape"), this);
	connect(escape_shortcut, &QShortcut::activated, this, &editor_window_t::on_search_clear);

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

void editor_window_t::on_search_apply()
{
	const auto query = m_search_field->text().toStdString();
	const bool search_in_id = m_search_id_btn->isChecked();
	const bool search_in_name = m_search_name_btn->isChecked();
	const bool case_sensitive = m_case_sensitive_btn->isChecked();
	const bool regex_mode = m_regex_btn->isChecked();

	m_plugin_workspace_view->apply_search(query, search_in_id, search_in_name, case_sensitive, regex_mode);
}

void editor_window_t::on_search_clear()
{
	if (m_search_field->text().isEmpty())
		return;

	m_search_field->clear();
	on_search_apply();
}

void editor_window_t::closeEvent(QCloseEvent * event)
{
	save_config();
	event->accept();
}

void editor_window_t::on_open_settings()
{
	editor_settings_dialog_t dialog(m_settings, this);
	connect(
	    &dialog,
	    &editor_settings_dialog_t::settings_applied,
	    m_plugin_workspace_view,
	    &plugin_workspace_view_t::on_settings_changed);

	if (dialog.exec() == QDialog::Accepted)
		m_plugin_workspace_view->on_settings_changed();
}
