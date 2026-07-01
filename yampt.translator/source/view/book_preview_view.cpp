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

	m_splitter = new QSplitter(Qt::Horizontal, this);

	auto * left_widget = new QWidget(m_splitter);
	auto * left_layout = new QVBoxLayout(left_widget);
	left_layout->setContentsMargins(0, 0, 0, 0);
	left_layout->setSpacing(0);
	m_original_browser = new QTextBrowser(left_widget);
	m_original_browser->setReadOnly(true);
	m_original_browser->setOpenLinks(false);
	m_original_browser->setHtml("<p style='color: gray;'>Select a book record</p>");
	left_layout->addWidget(m_original_browser);

	auto * right_widget = new QWidget(m_splitter);
	auto * right_layout = new QVBoxLayout(right_widget);
	right_layout->setContentsMargins(0, 0, 0, 0);
	right_layout->setSpacing(0);
	m_translation_browser = new QTextBrowser(right_widget);
	m_translation_browser->setReadOnly(true);
	m_translation_browser->setOpenLinks(false);
	m_translation_browser->setHtml("<p style='color: gray;'>Select a book record</p>");
	right_layout->addWidget(m_translation_browser);

	QFont font("Segoe UI", 10);
	m_original_browser->setFont(font);
	m_translation_browser->setFont(font);

	m_splitter->addWidget(left_widget);
	m_splitter->addWidget(right_widget);
	m_splitter->setSizes({ 500, 500 });

	layout->addWidget(m_splitter);
}

void book_preview_view_t::set_html(const std::string & original_html, const std::string & translation_html)
{
	m_original_browser->setHtml(prepare_html(original_html));
	m_translation_browser->setHtml(prepare_html(translation_html));
}

void book_preview_view_t::clear()
{
	m_original_browser->setHtml("<p style='color: gray;'>Select a book record</p>");
	m_translation_browser->setHtml("<p style='color: gray;'>Select a book record</p>");
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
