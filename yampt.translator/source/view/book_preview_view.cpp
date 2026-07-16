#include "book_preview_view.hpp"
#include "../highlighter/script_tokenizer.hpp"
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QScrollBar>
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

	m_syncing = false;

	auto sync_from_browser = [this](QTextBrowser * source_browser, QTextBrowser * target_browser)
	{
		if (m_syncing || !m_scroll_sync_enabled)
			return;

		m_syncing = true;
		auto * source_v = source_browser->verticalScrollBar();
		auto * target_v = target_browser->verticalScrollBar();
		if (source_v->maximum() > 0)
			target_v->setValue(static_cast<int>(static_cast<double>(source_v->value()) / source_v->maximum() * target_v->maximum()));

		target_browser->horizontalScrollBar()->setValue(source_browser->horizontalScrollBar()->value());
		m_syncing = false;
	};

	connect(m_original_browser->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from_browser]() { sync_from_browser(m_original_browser, m_translation_browser); });
	connect(m_original_browser->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from_browser]() { sync_from_browser(m_original_browser, m_translation_browser); });
	connect(m_translation_browser->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from_browser]() { sync_from_browser(m_translation_browser, m_original_browser); });
	connect(m_translation_browser->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from_browser]() { sync_from_browser(m_translation_browser, m_original_browser); });
}

void book_preview_view_t::set_scroll_sync(bool enabled)
{
	m_scroll_sync_enabled = enabled;
}

void book_preview_view_t::set_html(const std::string & original_html, const std::string & translation_html)
{
	m_original_browser->setHtml(prepare_html(original_html));
	m_translation_browser->setHtml(prepare_html(translation_html));
}

void book_preview_view_t::set_script(const std::string & original_script, const std::string & translated_script)
{
	m_original_browser->setHtml(highlight_script(original_script));
	m_translation_browser->setHtml(highlight_script(translated_script));
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

QString book_preview_view_t::highlight_script(const std::string & script) const
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize(script, rec_type_t::sctx);

	QString result;
	size_t last_end = 0;

	for (const auto & token : tokens)
	{
		if (token.start > last_end)
		{
			auto segment = QString::fromStdString(script.substr(last_end, token.start - last_end));
			result += segment.toHtmlEscaped();
		}

		auto segment = QString::fromStdString(script.substr(token.start, token.end - token.start));
		auto escaped = segment.toHtmlEscaped();

		switch (token.type)
		{
		case token_type_t::mwscript_function:
			result += "<span style='color: #569cd6;'>" + escaped + "</span>";
			break;
		case token_type_t::mwscript_string:
			result += "<span style='color: #ce9178;'>" + escaped + "</span>";
			break;
		case token_type_t::mwscript_comment:
			result += "<span style='color: #6a9955;'>" + escaped + "</span>";
			break;
		default:
			result += escaped;
			break;
		}

		last_end = token.end;
	}

	if (last_end < script.size())
	{
		auto tail = QString::fromStdString(script.substr(last_end));
		result += tail.toHtmlEscaped();
	}

	return "<pre style='font-family: Consolas, monospace; font-size: 10pt; margin: 4px;'>" + result + "</pre>";
}
