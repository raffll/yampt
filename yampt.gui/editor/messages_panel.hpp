#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <chrono>
#include <string>

class messages_panel_t : public QWidget
{
	Q_OBJECT

public:
	explicit messages_panel_t(QWidget * parent = nullptr);

	void log(const std::string & message);
	void clear();

private:
	QPlainTextEdit * text_ = nullptr;
	std::chrono::steady_clock::time_point start_time_;
};
