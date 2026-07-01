#include "topic_highlighter.hpp"
#include <algorithm>

topic_highlighter_t::topic_highlighter_t(QTextDocument * parent)
    : QSyntaxHighlighter(parent)
{
	m_format.setBackground(QColor(200, 220, 255));
}

void topic_highlighter_t::set_terms(const std::vector<std::string> & translated_terms)
{
	m_terms = translated_terms;

	if (document())
		rehighlight();
}

void topic_highlighter_t::highlightBlock(const QString & text)
{
	if (m_terms.empty())
		return;

	auto text_str = text.toStdString();
	auto text_lower = text_str;
	std::transform(
	    text_lower.begin(),
	    text_lower.end(),
	    text_lower.begin(),
	    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	for (const auto & term : m_terms)
	{
		std::string term_lower = term;
		std::transform(
		    term_lower.begin(),
		    term_lower.end(),
		    term_lower.begin(),
		    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (term_lower.empty())
			continue;

		size_t pos = 0;
		while ((pos = text_lower.find(term_lower, pos)) != std::string::npos)
		{
			setFormat(static_cast<int>(pos), static_cast<int>(term_lower.size()), m_format);
			pos += term_lower.size();
		}
	}
}
