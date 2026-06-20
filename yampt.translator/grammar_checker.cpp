#include "grammar_checker.hpp"
#include "editor_text_edit.hpp"

#include <QTextCursor>
#include <QTextDocument>

QList<QTextEdit::ExtraSelection> grammar_checker_t::check(editor_text_edit_t * editor, tools_t::rec_type_t type) const
{
	QList<QTextEdit::ExtraSelection> selections;

	const auto text = editor->document()->toPlainText();
	if (text.isEmpty())
		return selections;

	QTextCharFormat fmt;
	fmt.setBackground(QColor(200, 150, 0, 60));

	for (int i = 0; i < text.size() - 1; ++i)
	{
		if (text[i] != ' ' || text[i + 1] != ' ')
			continue;

		int start = i + 1;
		while (i + 1 < text.size() && text[i + 1] == ' ')
			++i;

		QTextEdit::ExtraSelection sel;
		sel.format = fmt;
		sel.cursor = QTextCursor(editor->document());
		sel.cursor.setPosition(start);
		sel.cursor.setPosition(i + 1, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

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

	if (quote_count % 2 != 0 && last_quote >= 0)
	{
		QTextEdit::ExtraSelection sel;
		sel.format = fmt;
		sel.cursor = QTextCursor(editor->document());
		sel.cursor.setPosition(last_quote);
		sel.cursor.setPosition(last_quote + 1, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

	int open_count = 0;
	int close_count = 0;
	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '(')
			++open_count;
		else if (text[i] == ')')
			++close_count;
	}

	if (open_count != close_count)
	{
		for (int i = 0; i < text.size(); ++i)
		{
			if (text[i] != '(' && text[i] != ')')
				continue;

			QTextEdit::ExtraSelection sel;
			sel.format = fmt;
			sel.cursor = QTextCursor(editor->document());
			sel.cursor.setPosition(i);
			sel.cursor.setPosition(i + 1, QTextCursor::KeepAnchor);
			selections.append(sel);
		}
	}

	if (type == tools_t::rec_type_t::info || type == tools_t::rec_type_t::text)
	{
		const auto last = text[text.size() - 1];
		if (!last.isSpace() && last != '.' && last != '!' && last != '?' && last != '"' && last != ')')
		{
			QTextEdit::ExtraSelection sel;
			sel.format = fmt;
			sel.cursor = QTextCursor(editor->document());
			sel.cursor.setPosition(text.size() - 1);
			sel.cursor.setPosition(text.size(), QTextCursor::KeepAnchor);
			selections.append(sel);
		}
	}

	return selections;
}
