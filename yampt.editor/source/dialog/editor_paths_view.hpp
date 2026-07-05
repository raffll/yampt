#pragma once

#include <QWidget>

class QLineEdit;
class settings_store_t;

class editor_paths_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit editor_paths_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void apply(settings_store_t & settings) const;

private:
	QLineEdit * m_edt_folder_path = nullptr;
	QLineEdit * m_edt_mo2_path = nullptr;
	QLineEdit * m_edt_openmw_path = nullptr;
};
