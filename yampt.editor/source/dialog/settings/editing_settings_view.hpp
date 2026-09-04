#pragma once

#include <QWidget>

class QCheckBox;
class settings_store_t;

class editing_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit editing_settings_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void save(settings_store_t & settings) const;

private:
	QCheckBox * m_editing_check = nullptr;
};
