#pragma once

#include <QWidget>

class QLineEdit;
class app_settings_t;

class editor_paths_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit editor_paths_view_t(QWidget * parent = nullptr);

	void load(const app_settings_t & settings);
	void apply(app_settings_t & settings) const;

private:
	QLineEdit * m_openmw_merge_path_edit = nullptr;
	QLineEdit * m_mo2_merge_path_edit = nullptr;
	QLineEdit * m_merge_filename_edit = nullptr;
};
