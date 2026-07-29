#pragma once

#include <utility/theme_enums.hpp>
#include <QWidget>

class QComboBox;
class settings_store_t;

class appearance_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit appearance_settings_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void save(settings_store_t & settings) const;
	bool is_modified() const;

private:
	QComboBox * m_theme_combo = nullptr;
	theme_t m_initial_theme = theme_t::light;
};
