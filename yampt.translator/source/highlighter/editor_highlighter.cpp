#include "editor_highlighter.hpp"
#include "../utility/spell_checker.hpp"

editor_highlighter_t::editor_highlighter_t(QTextDocument * parent)
    : QSyntaxHighlighter(parent)
{
	m_format_function.setForeground(QColor(100, 180, 255));
	m_format_comment.setForeground(QColor(128, 128, 128));
	m_format_string.setForeground(QColor(200, 150, 50));
	m_format_html_tag.setForeground(QColor(100, 0, 20));
	m_format_html_tag.setFontWeight(QFont::Bold);
	m_format_forbidden.setBackground(QColor(255, 200, 180));
	m_format_hyperlink.setForeground(QColor(70, 130, 220));
	m_format_hyperlink.setFontUnderline(true);
	m_format_misspelled.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	m_format_misspelled.setUnderlineColor(QColor(220, 50, 50));
}

void editor_highlighter_t::set_record_type(tools_t::rec_type_t type)
{
	m_record_type = type;

	if (document())
		rehighlight();
}

void editor_highlighter_t::set_translation_mode(bool enabled)
{
	m_is_translation = enabled;
}

void editor_highlighter_t::set_spell_checker(spell_checker_t * checker)
{
	m_spell_checker = checker;

	if (document())
		rehighlight();
}

void editor_highlighter_t::apply_syntax_tokens(const QString & text)
{
	const auto utf8 = text.toStdString();
	const auto & tokens = m_tokenizer.tokenize(utf8, m_record_type);

	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::normal)
			continue;

		const int start = QString::fromUtf8(utf8.data(), static_cast<int>(token.start)).length();
		const int end = QString::fromUtf8(utf8.data(), static_cast<int>(token.end)).length();
		const int length = end - start;

		switch (token.type)
		{
		case token_type_t::mwscript_function:
			setFormat(start, length, m_format_function);
			break;
		case token_type_t::mwscript_comment:
			setFormat(start, length, m_format_comment);
			break;
		case token_type_t::mwscript_string:
			setFormat(start, length, m_format_string);
			break;
		case token_type_t::html_tag:
			setFormat(start, length, m_format_html_tag);
			break;
		default:
			break;
		}
	}
}

void editor_highlighter_t::apply_forbidden_chars(const QString & text)
{
	for (int i = 0; i < text.length(); ++i)
	{
		const auto ch = text.at(i).unicode();

		const bool is_standalone_at = ch == '@' && (i + 1 >= text.length() || !text.at(i + 1).isLetterOrNumber());

		if (ch == '|' || ch == '~' || is_standalone_at || ch == '{' || ch == '}' ||
		    (ch <= 0x1F && ch != 0x09 && ch != 0x0D && ch != 0x0A))
		{
			auto merged = format(i);
			merged.setBackground(QColor(255, 200, 180));
			setFormat(i, 1, merged);
		}
	}
}

void editor_highlighter_t::apply_hyperlink_prefix(const QString & text)
{
	for (int i = 0; i < text.length() - 1; ++i)
	{
		if (text.at(i).unicode() != '@')
			continue;

		if (!text.at(i + 1).isLetterOrNumber())
			continue;

		int end = i + 1;
		while (end < text.length() && text.at(end).isLetterOrNumber())
			++end;

		setFormat(i, end - i, m_format_hyperlink);
		i = end - 1;
	}
}

void editor_highlighter_t::apply_spell_check(const QString & text)
{
	const auto & text_str = text.toStdString();
	const auto & matches = m_spell_checker->find_misspelled(text_str);

	for (const auto & match : matches)
	{
		const int qchar_start = QString::fromUtf8(text_str.data(), static_cast<int>(match.start)).length();
		const int qchar_len =
		    QString::fromUtf8(text_str.data() + match.start, static_cast<int>(match.end - match.start)).length();
		setFormat(qchar_start, qchar_len, m_format_misspelled);
	}
}

void editor_highlighter_t::highlightBlock(const QString & text)
{
	const bool has_syntax = m_record_type == tools_t::rec_type_t::sctx || m_record_type == tools_t::rec_type_t::bnam ||
	                        m_record_type == tools_t::rec_type_t::text;

	if (has_syntax)
		apply_syntax_tokens(text);

	apply_forbidden_chars(text);
	apply_hyperlink_prefix(text);

	if (!m_is_translation || !m_spell_checker || !m_spell_checker->is_loaded())
		return;

	apply_spell_check(text);
}
