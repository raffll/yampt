#include "editor_highlighter.hpp"
#include "../utility/spell_checker.hpp"

editor_highlighter_t::editor_highlighter_t(QTextDocument * parent)
    : QSyntaxHighlighter(parent)
{
	format_function_.setForeground(QColor(100, 180, 255));
	format_comment_.setForeground(QColor(128, 128, 128));
	format_string_.setForeground(QColor(200, 150, 50));
	format_html_tag_.setForeground(QColor(100, 0, 20));
	format_html_tag_.setFontWeight(QFont::Bold);
	format_forbidden_.setBackground(QColor(255, 200, 180));
	format_misspelled_.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	format_misspelled_.setUnderlineColor(QColor(220, 50, 50));
}

void editor_highlighter_t::set_record_type(tools_t::rec_type_t type)
{
	record_type_ = type;

	if (document())
		rehighlight();
}

void editor_highlighter_t::set_translation_mode(bool enabled)
{
	is_translation_ = enabled;
}

void editor_highlighter_t::set_spell_checker(spell_checker_t * checker)
{
	spell_checker_ = checker;

	if (document())
		rehighlight();
}

void editor_highlighter_t::apply_syntax_tokens(const QString & text)
{
	const auto utf8 = text.toStdString();
	const auto & tokens = tokenizer_.tokenize(utf8, record_type_);

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
			setFormat(start, length, format_function_);
			break;
		case token_type_t::mwscript_comment:
			setFormat(start, length, format_comment_);
			break;
		case token_type_t::mwscript_string:
			setFormat(start, length, format_string_);
			break;
		case token_type_t::html_tag:
			setFormat(start, length, format_html_tag_);
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

		if (ch == '|' || ch == '~' || ch == '@' || ch == '{' || ch == '}' ||
		    (ch <= 0x1F && ch != 0x09 && ch != 0x0D && ch != 0x0A))
		{
			auto merged = format(i);
			merged.setBackground(QColor(255, 200, 180));
			setFormat(i, 1, merged);
		}
	}
}

void editor_highlighter_t::apply_spell_check(const QString & text)
{
	const auto & text_str = text.toStdString();
	const auto & matches = spell_checker_->find_misspelled(text_str);

	for (const auto & match : matches)
	{
		const int qchar_start = QString::fromUtf8(text_str.data(), static_cast<int>(match.start)).length();
		const int qchar_len =
		    QString::fromUtf8(text_str.data() + match.start, static_cast<int>(match.end - match.start)).length();
		setFormat(qchar_start, qchar_len, format_misspelled_);
	}
}

void editor_highlighter_t::highlightBlock(const QString & text)
{
	const bool has_syntax = record_type_ == tools_t::rec_type_t::sctx || record_type_ == tools_t::rec_type_t::bnam ||
	                        record_type_ == tools_t::rec_type_t::text;

	if (has_syntax)
		apply_syntax_tokens(text);

	apply_forbidden_chars(text);

	if (!is_translation_ || !spell_checker_ || !spell_checker_->is_loaded())
		return;

	apply_spell_check(text);
}
