#include "book_preview.hpp"

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

    layout->addWidget(browser_);
}

void book_preview_t::set_html(const std::string & html)
{
    browser_->setHtml(QString::fromStdString(html));
}

void book_preview_t::clear()
{
    browser_->setHtml("<p style='color: gray;'>Select a book record</p>");
}
