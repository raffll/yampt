#pragma once

#include <string>
#include <QWidget>

class QTextEdit;

class preview_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit preview_view_t(QWidget * parent = nullptr);

	void show_comparison(const std::string & left_text, const std::string & right_text);
	void clear();

private:
	QTextEdit * m_left_edit = nullptr;
	QTextEdit * m_right_edit = nullptr;
};
