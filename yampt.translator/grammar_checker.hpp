#pragma once

#include <QTextEdit>

class editor_text_edit_t;

class grammar_checker_t
{
public:
	grammar_checker_t() = default;

	QList<QTextEdit::ExtraSelection> check(editor_text_edit_t * editor) const;
};
