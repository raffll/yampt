#pragma once

#include <string>
#include <QWidget>

class QCheckBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTabWidget;
class settings_store_t;

class merge_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit merge_settings_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void save(settings_store_t & settings) const;

private:
	void setup_ignore_fields_tab();
	void setup_exclude_by_id_tab();
	void setup_fixes_tab();

	void on_ignore_add();
	void on_ignore_remove();
	void on_exclude_add();
	void on_exclude_remove();

	QTabWidget * m_tabs = nullptr;

	QListWidget * m_ignore_list = nullptr;
	QLineEdit * m_ignore_input = nullptr;
	QPushButton * m_ignore_add_button = nullptr;
	QPushButton * m_ignore_remove_button = nullptr;

	QListWidget * m_exclude_list = nullptr;
	QLineEdit * m_exclude_input = nullptr;
	QPushButton * m_exclude_add_button = nullptr;
	QPushButton * m_exclude_remove_button = nullptr;

	QCheckBox * m_fog_fix_check = nullptr;
	QCheckBox * m_summon_fix_check = nullptr;
	QCheckBox * m_cell_name_fix_check = nullptr;
};
