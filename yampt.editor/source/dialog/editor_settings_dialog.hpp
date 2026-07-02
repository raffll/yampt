#pragma once

#include <string>
#include <QDialog>

class QDialogButtonBox;
class QListWidget;
class QPushButton;
class QStackedWidget;
class app_settings_t;
class appearance_settings_view_t;
class editor_paths_view_t;

class editor_settings_dialog_t : public QDialog
{
	Q_OBJECT

public:
	explicit editor_settings_dialog_t(app_settings_t & settings, QWidget * parent = nullptr);

signals:
	void settings_applied(const std::string & category);

private:
	void apply_all();

	app_settings_t & m_settings;

	QListWidget * m_category_list = nullptr;
	QStackedWidget * m_content_stack = nullptr;
	QDialogButtonBox * m_button_box = nullptr;
	QPushButton * m_apply_button = nullptr;

	appearance_settings_view_t * m_appearance_view = nullptr;
	editor_paths_view_t * m_paths_view = nullptr;
};
