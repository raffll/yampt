#pragma once

#include <string>
#include <QWidget>

class QCheckBox;
class QLineEdit;
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
	void setup_exclude_tab();
	void setup_fixes_tab();

	QTabWidget * m_tabs = nullptr;
	QLineEdit * m_ignore_fields_edit = nullptr;
	QLineEdit * m_exclusion_edit = nullptr;
	QCheckBox * m_fog_fix_check = nullptr;
	QCheckBox * m_summon_fix_check = nullptr;
	QCheckBox * m_cell_name_fix_check = nullptr;
};
