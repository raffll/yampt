#pragma once

#include <QWidget>

class QCheckBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;

class find_replace_dialog_t : public QWidget
{
	Q_OBJECT

public:
	explicit find_replace_dialog_t(QWidget * parent = nullptr);

signals:
	void find_next_requested(const QString & query, bool case_sensitive, bool regex_mode);
	void replace_requested(const QString & query, const QString & replacement, bool case_sensitive, bool regex_mode);
	void replace_all_requested(
	    const QString & query,
	    const QString & replacement,
	    bool case_sensitive,
	    bool regex_mode);

private:
	void setup_layout(QGridLayout * layout);
	void connect_signals();

	QLineEdit * m_find_field = nullptr;
	QLineEdit * m_replace_field = nullptr;
	QCheckBox * m_case_check = nullptr;
	QCheckBox * m_regex_check = nullptr;
	QPushButton * m_find_next_btn = nullptr;
	QPushButton * m_replace_btn = nullptr;
	QPushButton * m_replace_all_btn = nullptr;
	QLabel * m_note_label = nullptr;
};
