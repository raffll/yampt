#pragma once

#include <settings_store.hpp>
#include <QMainWindow>

class QAction;
class QLineEdit;
class QToolButton;
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
	void on_search_apply();
	void on_search_clear();

	plugin_workspace_view_t * m_plugin_workspace_view = nullptr;
	settings_store_t m_settings { "yEditor.ini" };

	QLineEdit * m_search_field = nullptr;
	QToolButton * m_case_sensitive_btn = nullptr;
	QToolButton * m_regex_btn = nullptr;
	QToolButton * m_search_id_btn = nullptr;
	QToolButton * m_search_name_btn = nullptr;
};
