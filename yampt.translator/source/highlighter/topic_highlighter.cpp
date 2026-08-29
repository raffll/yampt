#include "topic_highlighter.hpp"
#include <utility/string_utils.hpp>
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

	const auto text_lower = string_utils::to_lower_utf8(text.toStdString());

	for (const auto & term : m_terms)
	{
		const auto term_lower = string_utils::to_lower_utf8(term);

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
