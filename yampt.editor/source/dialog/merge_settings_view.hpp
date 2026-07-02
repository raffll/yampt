#pragma once

#include <string>
#include <unordered_map>
#include <QWidget>

class QCheckBox;
class QLineEdit;
class app_settings_t;

class merge_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit merge_settings_view_t(QWidget * parent = nullptr);

	void load(const app_settings_t & settings);
	void save(app_settings_t & settings) const;

private:
	void setup_record_types_group();
	void setup_exclusion_group();
	void setup_fixes_group();

	std::unordered_map<std::string, QCheckBox *> m_type_checkboxes;
	QLineEdit * m_exclusion_edit = nullptr;
	QCheckBox * m_fog_fix_check = nullptr;
	QCheckBox * m_summon_fix_check = nullptr;
	QCheckBox * m_cell_name_fix_check = nullptr;
};
