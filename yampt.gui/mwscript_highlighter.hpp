#pragma once

#include "../yampt/tools.hpp"
#include "syntax_highlighter.hpp"
#include <QSyntaxHighlighter>

class mwscript_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit mwscript_highlighter_t(QTextDocument * parent = nullptr);

	void set_record_type(tools_t::rec_type_t type);

protected:
	void highlightBlock(const QString & text) override;

private:
	tools_t::rec_type_t record_type_ = tools_t::rec_type_t::unknown;
	syntax_highlighter_t tokenizer_;

	QTextCharFormat format_function_;
	QTextCharFormat format_comment_;
	QTextCharFormat format_string_;
	QTextCharFormat format_html_tag_;
};
