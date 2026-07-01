#pragma once

#include <chrono>
#include <string>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class messages_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit messages_view_t(QWidget * parent = nullptr);

	void log(const std::string & message);
	void clear();

private:
	QPlainTextEdit * m_text = nullptr;
	std::chrono::steady_clock::time_point m_start_time;
};
