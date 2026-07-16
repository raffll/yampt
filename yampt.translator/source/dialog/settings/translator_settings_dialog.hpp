#pragma once

#include <string>
#include <QDialog>

class QDialogButtonBox;
class QListWidget;
class QPushButton;
class QStackedWidget;
class settings_store_t;
class appearance_settings_view_t;
class language_settings_view_t;
class translation_settings_view_t;
class shortcuts_settings_view_t;

class translator_settings_dialog_t : public QDialog
{
	Q_OBJECT

public:
	explicit translator_settings_dialog_t(
	    settings_store_t & settings,
	    const std::string & dictionaries_dir,
	    QWidget * parent = nullptr);

signals:
	void settings_applied(const std::string & changed_category);

private:
	void apply_all();
	void update_ok_button_state();

	settings_store_t & m_settings;

	QListWidget * m_category_list = nullptr;
	QStackedWidget * m_content_stack = nullptr;
	QDialogButtonBox * m_button_box = nullptr;
	QPushButton * m_apply_button = nullptr;

	appearance_settings_view_t * m_appearance_view = nullptr;
	language_settings_view_t * m_language_view = nullptr;
	translation_settings_view_t * m_translation_view = nullptr;
	shortcuts_settings_view_t * m_shortcuts_view = nullptr;
};
