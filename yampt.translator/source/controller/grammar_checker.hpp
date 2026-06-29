#pragma once

#include <utility/tools.hpp>
#include <QTextEdit>

class translation_edit_view_t;

class grammar_checker_t
{
public:
	grammar_checker_t() = default;

	QList<QTextEdit::ExtraSelection> check(translation_edit_view_t * editor, tools_t::rec_type_t type) const;

private:
	void check_double_spaces(
	    QList<QTextEdit::ExtraSelection> & selections,
	    const QString & text,
	    QTextDocument * document) const;

	void check_unmatched_quotes(
	    QList<QTextEdit::ExtraSelection> & selections,
	    const QString & text,
	    QTextDocument * document) const;

	void check_unmatched_parens(
	    QList<QTextEdit::ExtraSelection> & selections,
	    const QString & text,
	    QTextDocument * document) const;

	void check_missing_punctuation(
	    QList<QTextEdit::ExtraSelection> & selections,
	    const QString & text,
	    QTextDocument * document,
	    tools_t::rec_type_t type) const;

	static QTextCharFormat warning_format();
};
