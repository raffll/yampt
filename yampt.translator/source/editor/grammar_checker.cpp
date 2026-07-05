#include "grammar_checker.hpp"
#include "../view/translation_edit_view.hpp"
#include <QTextCursor>
#include <QTextDocument>

QTextCharFormat grammar_checker_t::warning_format()
{
	QTextCharFormat fmt;
	fmt.setBackground(QColor(200, 150, 0, 60));
	return fmt;
}

QList<QTextEdit::ExtraSelection> grammar_checker_t::check(translation_edit_view_t * editor, tools_t::rec_type_t type)
    const
{
	QList<QTextEdit::ExtraSelection> selections;

	const auto text = editor->document()->toPlainText();
	if (text.isEmpty())
		return selections;

	auto * document = editor->document();

	check_double_spaces(selections, text, document);
	check_unmatched_quotes(selections, text, document);
	check_unmatched_parens(selections, text, document);
	check_missing_punctuation(selections, text, document, type);

	return selections;
}

void grammar_checker_t::check_double_spaces(
    QList<QTextEdit::ExtraSelection> & selections,
    const QString & text,
    QTextDocument * document) const
{
	const auto fmt = warning_format();

	for (int i = 0; i < text.size() - 1; ++i)
	{
		if (text[i] != ' ' || text[i + 1] != ' ')
			continue;

		int start = i + 1;
		while (i + 1 < text.size() && text[i + 1] == ' ')
			++i;

		QTextEdit::ExtraSelection sel;
		sel.format = fmt;
		sel.cursor = QTextCursor(document);
		sel.cursor.setPosition(start);
		sel.cursor.setPosition(i + 1, QTextCursor::KeepAnchor);
		selections.append(sel);
	}
}

void grammar_checker_t::check_unmatched_quotes(
    QList<QTextEdit::ExtraSelection> & selections,
    const QString & text,
    QTextDocument * document) const
{
	int quote_count = 0;
	int last_quote = -1;
	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '"')
		{
			++quote_count;
			last_quote = i;
		}
	}

	if (quote_count % 2 == 0 || last_quote < 0)
		return;

	QTextEdit::ExtraSelection sel;
	sel.format = warning_format();
	sel.cursor = QTextCursor(document);
	sel.cursor.setPosition(last_quote);
	sel.cursor.setPosition(last_quote + 1, QTextCursor::KeepAnchor);
	selections.append(sel);
}

void grammar_checker_t::check_unmatched_parens(
    QList<QTextEdit::ExtraSelection> & selections,
    const QString & text,
    QTextDocument * document) const
{
	int open_count = 0;
	int close_count = 0;
	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '(')
			++open_count;
		else if (text[i] == ')')
			++close_count;
	}

	if (open_count == close_count)
		return;

	const auto fmt = warning_format();

	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] != '(' && text[i] != ')')
			continue;

		QTextEdit::ExtraSelection sel;
		sel.format = fmt;
		sel.cursor = QTextCursor(document);
		sel.cursor.setPosition(i);
		sel.cursor.setPosition(i + 1, QTextCursor::KeepAnchor);
		selections.append(sel);
	}
}

void grammar_checker_t::check_missing_punctuation(
    QList<QTextEdit::ExtraSelection> & selections,
    const QString & text,
    QTextDocument * document,
    tools_t::rec_type_t type) const
{
	if (type != tools_t::rec_type_t::info && type != tools_t::rec_type_t::text)
		return;

	const auto last = text[text.size() - 1];
	if (last.isSpace() || last == '.' || last == '!' || last == '?' || last == '"' || last == ')')
		return;

	QTextEdit::ExtraSelection sel;
	sel.format = warning_format();
	sel.cursor = QTextCursor(document);
	sel.cursor.setPosition(text.size() - 1);
	sel.cursor.setPosition(text.size(), QTextCursor::KeepAnchor);
	selections.append(sel);
}
