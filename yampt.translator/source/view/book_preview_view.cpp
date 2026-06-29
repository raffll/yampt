#include "book_preview_view.hpp"

#include <QRegularExpression>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>

book_preview_view_t::book_preview_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	splitter_ = new QSplitter(Qt::Horizontal, this);

	auto * left_widget = new QWidget(splitter_);
	auto * left_layout = new QVBoxLayout(left_widget);
	left_layout->setContentsMargins(0, 0, 0, 0);
	left_layout->setSpacing(0);
	original_browser_ = new QTextBrowser(left_widget);
	original_browser_->setReadOnly(true);
	original_browser_->setOpenLinks(false);
	original_browser_->setHtml("<p style='color: gray;'>Select a book record</p>");
	left_layout->addWidget(original_browser_);

	auto * right_widget = new QWidget(splitter_);
	auto * right_layout = new QVBoxLayout(right_widget);
	right_layout->setContentsMargins(0, 0, 0, 0);
	right_layout->setSpacing(0);
	translation_browser_ = new QTextBrowser(right_widget);
	translation_browser_->setReadOnly(true);
	translation_browser_->setOpenLinks(false);
	translation_browser_->setHtml("<p style='color: gray;'>Select a book record</p>");
	right_layout->addWidget(translation_browser_);

	QFont font("Segoe UI", 10);
	original_browser_->setFont(font);
	translation_browser_->setFont(font);

	splitter_->addWidget(left_widget);
	splitter_->addWidget(right_widget);
	splitter_->setSizes({ 500, 500 });

	layout->addWidget(splitter_);
}

void book_preview_view_t::set_html(const std::string & original_html, const std::string & translation_html)
{
	original_browser_->setHtml(prepare_html(original_html));
	translation_browser_->setHtml(prepare_html(translation_html));
}

void book_preview_view_t::clear()
{
	original_browser_->setHtml("<p style='color: gray;'>Select a book record</p>");
	translation_browser_->setHtml("<p style='color: gray;'>Select a book record</p>");
}

QString book_preview_view_t::prepare_html(const std::string & html) const
{
	auto content = QString::fromStdString(html);

	static QRegularExpression face_re("FACE\\s*=\\s*\"[^\"]*\"", QRegularExpression::CaseInsensitiveOption);
	content.replace(face_re, "");

	static QRegularExpression color_re("COLOR\\s*=\\s*\"([0-9A-Fa-f]{6})\"", QRegularExpression::CaseInsensitiveOption);
	content.replace(color_re, "COLOR=\"#\\1\"");

	return content;
}
