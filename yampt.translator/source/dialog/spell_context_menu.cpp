#include "spell_context_menu.hpp"
#include "../utility/spell_checker.hpp"
#include "../view/translation_edit_view.hpp"
#include <QAction>
#include <QMenu>
#include <QSyntaxHighlighter>
#include <QTextCursor>

spell_context_menu_t::spell_context_menu_t(spell_checker_t * checker, QSyntaxHighlighter * highlighter)
    : checker_(checker)
    , highlighter_(highlighter)
{}

void spell_context_menu_t::show_menu(translation_edit_view_t * editor, const QPoint & pos)
{
	int start = 0;
	int end = 0;
	auto word = get_word_at_cursor(editor, pos, start, end);

	if (word.empty())
		return;

	if (checker_->check_word(word))
		return;

	auto suggestions = checker_->suggest(word);
	if (suggestions.size() > 10)
		suggestions.resize(10);

	QMenu menu;

	for (const auto & suggestion : suggestions)
		menu.addAction(QString::fromStdString(suggestion));

	menu.addSeparator();
	auto * add_action = menu.addAction("Add to dictionary");

	auto * selected = menu.exec(editor->mapToGlobal(pos));
	if (!selected)
		return;

	if (selected == add_action)
	{
		checker_->add_to_user_dict(word);
		highlighter_->rehighlight();
		return;
	}

	QTextCursor cursor = editor->textCursor();
	cursor.setPosition(start);
	cursor.setPosition(end, QTextCursor::KeepAnchor);
	cursor.insertText(selected->text());
}

std::string spell_context_menu_t::get_word_at_cursor(
    translation_edit_view_t * editor,
    const QPoint & pos,
    int & start,
    int & end) const
{
	QTextCursor cursor = editor->cursorForPosition(pos);
	int position = cursor.position();

	auto doc = editor->document();
	auto text = doc->toPlainText();
	int length = text.length();

	auto is_word_char = [&](int i) -> bool
	{
		auto ch = text.at(i);
		if (ch.isLetter())
			return true;

		return ch.unicode() >= 0x80;
	};

	if (position >= length || !is_word_char(position))
	{
		if (position > 0 && is_word_char(position - 1))
			position--;
		else
			return {};
	}

	start = position;
	while (start > 0 && is_word_char(start - 1))
		start--;

	end = position;
	while (end < length && is_word_char(end))
		end++;

	auto selected_text = text.mid(start, end - start);
	return selected_text.toStdString();
}
