#include "log_tab.hpp"

#include <QDateTime>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCursor>
#include <QVBoxLayout>

log_tab_t::log_tab_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	text_edit_ = new QPlainTextEdit(this);
	text_edit_->setReadOnly(true);
	layout->addWidget(text_edit_);
}

void log_tab_t::append_log(const std::string & operation_name, const std::string & log_text)
{
	const auto & timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();

	auto header = "--- [" + operation_name + "] [" + timestamp + "] ---\n";

	text_edit_->appendPlainText(QString::fromStdString(header + log_text));
	text_edit_->verticalScrollBar()->setValue(text_edit_->verticalScrollBar()->maximum());
}

void log_tab_t::append_text(const std::string & text)
{
	text_edit_->moveCursor(QTextCursor::End);
	text_edit_->insertPlainText(QString::fromStdString(text));
	text_edit_->verticalScrollBar()->setValue(text_edit_->verticalScrollBar()->maximum());
}

void log_tab_t::clear()
{
	text_edit_->clear();
}
