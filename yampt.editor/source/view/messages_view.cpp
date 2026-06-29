#include "messages_view.hpp"
#include <QFont>
#include <QTextCursor>
#include <cstdio>

messages_view_t::messages_view_t(QWidget * parent)
    : QWidget(parent)
    , start_time_(std::chrono::steady_clock::now())
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	text_ = new QPlainTextEdit(this);
	text_->setReadOnly(true);

	QFont font("Consolas", 9);
	font.setStyleHint(QFont::Monospace);
	text_->setFont(font);

	layout->addWidget(text_);
}

void messages_view_t::log(const std::string & message)
{
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
	int minutes = static_cast<int>(elapsed.count()) / 60;
	int seconds = static_cast<int>(elapsed.count()) % 60;

	char timestamp[16];
	std::snprintf(timestamp, sizeof(timestamp), "[%02d:%02d] ", minutes, seconds);

	text_->appendPlainText(QString::fromStdString(std::string(timestamp) + message));
	text_->moveCursor(QTextCursor::End);
	text_->ensureCursorVisible();
}

void messages_view_t::clear()
{
	text_->clear();
	start_time_ = std::chrono::steady_clock::now();
}
