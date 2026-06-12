#include "mwscript_highlighter.hpp"
#include "syntax_highlighter.hpp"

mwscript_highlighter_t::mwscript_highlighter_t(QTextDocument * parent)
	: QSyntaxHighlighter(parent)
{
	format_function_.setForeground(QColor(100, 180, 255));
	format_comment_.setForeground(QColor(128, 128, 128));
	format_string_.setForeground(QColor(200, 150, 50));
	format_html_tag_.setForeground(QColor(140, 40, 50));
}

void mwscript_highlighter_t::set_record_type(tools_t::rec_type_t type)
{
	record_type_ = type;

	if (document())
		rehighlight();
}

void mwscript_highlighter_t::highlightBlock(const QString & text)
{
	if (record_type_ != tools_t::rec_type_t::sctx &&
		record_type_ != tools_t::rec_type_t::bnam &&
		record_type_ != tools_t::rec_type_t::text)
		return;

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
