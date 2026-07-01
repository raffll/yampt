#include "messages_view.hpp"
#include <cstdio>
#include <QFont>
#include <QTextCursor>

messages_view_t::messages_view_t(QWidget * parent)
    : QWidget(parent)
    , m_start_time(std::chrono::steady_clock::now())
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_text = new QPlainTextEdit(this);
	m_text->setReadOnly(true);

	QFont font("Consolas", 9);
	font.setStyleHint(QFont::Monospace);
	m_text->setFont(font);

	layout->addWidget(m_text);
}

void messages_view_t::log(const std::string & message)
{
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_time);
	int minutes = static_cast<int>(elapsed.count()) / 60;
	int seconds = static_cast<int>(elapsed.count()) % 60;

	char timestamp[16];
	std::snprintf(timestamp, sizeof(timestamp), "[%02d:%02d] ", minutes, seconds);

	m_text->appendPlainText(QString::fromStdString(std::string(timestamp) + message));
	m_text->moveCursor(QTextCursor::End);
	m_text->ensureCursorVisible();
}

void messages_view_t::clear()
{
	m_text->clear();
	m_start_time = std::chrono::steady_clock::now();
}
