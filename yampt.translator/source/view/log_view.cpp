#include "log_view.hpp"
#include <QDateTime>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCursor>
#include <QVBoxLayout>

log_view_t::log_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_text_edit = new QPlainTextEdit(this);
	m_text_edit->setReadOnly(true);
	layout->addWidget(m_text_edit);
}

void log_view_t::append_log(const std::string & operation_name, const std::string & log_text)
{
	const auto & timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();

	auto header = "--- [" + operation_name + "] [" + timestamp + "] ---\n";

	m_text_edit->appendPlainText(QString::fromStdString(header + log_text));
	m_text_edit->verticalScrollBar()->setValue(m_text_edit->verticalScrollBar()->maximum());
}

void log_view_t::append_text(const std::string & text)
{
	m_text_edit->moveCursor(QTextCursor::End);
	m_text_edit->insertPlainText(QString::fromStdString(text));
	m_text_edit->verticalScrollBar()->setValue(m_text_edit->verticalScrollBar()->maximum());
}

void log_view_t::clear()
{
	m_text_edit->clear();
}
