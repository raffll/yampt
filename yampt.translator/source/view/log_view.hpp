#pragma once

#include <string>
#include <QWidget>

class QPlainTextEdit;

class log_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit log_view_t(QWidget * parent = nullptr);

	void append_log(const std::string & operation_name, const std::string & log_text);
	void append_text(const std::string & text);
	void clear();

private:
	QPlainTextEdit * text_edit_ = nullptr;
};
