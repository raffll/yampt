#pragma once

#include <QWidget>

class QComboBox;
class settings_store_t;
enum class theme_t;

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
	QComboBox * m_codepage_combo = nullptr;
	theme_t m_initial_theme;
	int m_initial_codepage = 1252;
};
