#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QTableWidget;
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
	void setup_excludes_tab();
	void setup_fixes_tab();

	void on_exclude_add();
	void on_exclude_remove();
	bool row_exists(int kind_index, const QString & target) const;

	QTabWidget * m_tabs = nullptr;

	QTableWidget * m_exclude_table = nullptr;
	QComboBox * m_kind_combo = nullptr;
	QLineEdit * m_target_input = nullptr;
	QPushButton * m_exclude_add_button = nullptr;
	QPushButton * m_exclude_remove_button = nullptr;

	QCheckBox * m_fog_fix_check = nullptr;
	QCheckBox * m_summon_fix_check = nullptr;
	QCheckBox * m_cell_name_fix_check = nullptr;
};
