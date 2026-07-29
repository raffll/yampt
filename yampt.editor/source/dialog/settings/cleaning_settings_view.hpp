#pragma once

#include <QWidget>

class QCheckBox;
class settings_store_t;

class cleaning_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit cleaning_settings_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void save(settings_store_t & settings) const;

private:
	QCheckBox * m_evil_gmst_check = nullptr;
	QCheckBox * m_junk_cell_check = nullptr;
	QCheckBox * m_update_master_sizes_check = nullptr;
	QCheckBox * m_update_version_check = nullptr;
};
