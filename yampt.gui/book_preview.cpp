#include "book_preview.hpp"

#include <QRegularExpression>
#include <QTextBrowser>
#include <QVBoxLayout>

book_preview_t::book_preview_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	browser_ = new QTextBrowser(this);
	browser_->setReadOnly(true);
	browser_->setOpenLinks(false);
	browser_->setHtml("<p style='color: gray;'>Select a book record</p>");

	QFont font("Segoe UI", 10);
	browser_->setFont(font);

	layout->addWidget(browser_);
}

void book_preview_t::set_html(const std::string & html)
{
	auto content = QString::fromStdString(html);

	static QRegularExpression face_re("FACE\\s*=\\s*\"[^\"]*\"", QRegularExpression::CaseInsensitiveOption);
	content.replace(face_re, "");

	static QRegularExpression color_re("COLOR\\s*=\\s*\"([0-9A-Fa-f]{6})\"", QRegularExpression::CaseInsensitiveOption);
	content.replace(color_re, "COLOR=\"#\\1\"");

	browser_->setHtml(content);
}

void book_preview_t::clear()
{
	browser_->setHtml("<p style='color: gray;'>Select a book record</p>");
}
