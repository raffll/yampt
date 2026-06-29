#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <chrono>
#include <string>

class messages_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit messages_view_t(QWidget * parent = nullptr);

	void log(const std::string & message);
	void clear();

private:
	QPlainTextEdit * text_ = nullptr;
	std::chrono::steady_clock::time_point start_time_;
};
