#pragma once

#include <QWidget>

class QLineEdit;
class QToolButton;
class settings_store_t;

class translation_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit translation_settings_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void apply(settings_store_t & settings) const;

private:
	QLineEdit * m_deepl_key_edit = nullptr;
	QToolButton * m_deepl_reveal_button = nullptr;
	QLineEdit * m_google_key_edit = nullptr;
	QToolButton * m_google_reveal_button = nullptr;
};
