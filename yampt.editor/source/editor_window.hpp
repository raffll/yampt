#pragma once

#include <io/app_settings.hpp>
#include <QMainWindow>

class plugin_workspace_view_t;

class editor_window_t : public QMainWindow
{
	Q_OBJECT

public:
	explicit editor_window_t(QWidget * parent = nullptr);

protected:
	void closeEvent(QCloseEvent * event) override;

private:
	void setup_menu_bar();
	void setup_toolbar();
	void load_config();
	void save_config();
	void on_open_settings();

	plugin_workspace_view_t * m_plugin_workspace_view = nullptr;
	app_settings_t m_settings { "yEditor.ini" };
};
