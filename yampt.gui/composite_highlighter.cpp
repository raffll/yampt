#include "composite_highlighter.hpp"
#include "spell_checker.hpp"

composite_highlighter_t::composite_highlighter_t(QTextDocument * parent)
	: QSyntaxHighlighter(parent)
{
	format_function_.setForeground(QColor(100, 180, 255));
	format_comment_.setForeground(QColor(128, 128, 128));
	format_string_.setForeground(QColor(200, 150, 50));
	format_html_tag_.setForeground(QColor(140, 140, 150));
	format_forbidden_.setBackground(QColor(255, 200, 180));
	format_misspelled_.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	format_misspelled_.setUnderlineColor(QColor(220, 50, 50));
}

void composite_highlighter_t::set_record_type(tools_t::rec_type_t type)
{
	record_type_ = type;

	if (document())
		rehighlight();
}

void composite_highlighter_t::set_translation_mode(bool enabled)
{
	is_translation_ = enabled;
}

void composite_highlighter_t::set_spell_checker(spell_checker_t * checker)
{
	spell_checker_ = checker;

	if (document())
		rehighlight();
}

void composite_highlighter_t::highlightBlock(const QString & text)
{
	if (record_type_ == tools_t::rec_type_t::sctx ||
		record_type_ == tools_t::rec_type_t::bnam ||
		record_type_ == tools_t::rec_type_t::text)
	{
		const auto tokens = tokenizer_.tokenize(text.toStdString(), record_type_);

		for (const auto & token : tokens)
		{
			if (token.type == token_type_t::normal)
				continue;

			const int start = static_cast<int>(token.start);
			const int length = static_cast<int>(token.end - token.start);

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

		if (ch == '"' && (record_type_ == tools_t::rec_type_t::sctx || record_type_ == tools_t::rec_type_t::bnam))
		{
			auto merged = format(i);
			merged.setBackground(QColor(255, 200, 180));
			setFormat(i, 1, merged);
		}
	}

	if (!is_translation_ || !spell_checker_ || !spell_checker_->is_loaded())
		return;

	const auto text_str = text.toStdString();
	const auto matches = spell_checker_->find_misspelled(text_str);

	for (const auto & match : matches)
		setFormat(static_cast<int>(match.start), static_cast<int>(match.end - match.start), format_misspelled_);
}
