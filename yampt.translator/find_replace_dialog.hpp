#pragma once

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

class find_replace_dialog_t : public QDialog
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
	QLineEdit * find_field_ = nullptr;
	QLineEdit * replace_field_ = nullptr;
	QCheckBox * case_check_ = nullptr;
	QCheckBox * regex_check_ = nullptr;
	QPushButton * find_next_btn_ = nullptr;
	QPushButton * replace_btn_ = nullptr;
	QPushButton * replace_all_btn_ = nullptr;
	QLabel * note_label_ = nullptr;
};
