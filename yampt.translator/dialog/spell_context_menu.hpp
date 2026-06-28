#pragma once

#include <string>

class editor_text_edit_t;
class QPoint;
class QSyntaxHighlighter;
class spell_checker_t;

class spell_context_menu_t
{
public:
	spell_context_menu_t(spell_checker_t * checker, QSyntaxHighlighter * highlighter);

	void show_menu(editor_text_edit_t * editor, const QPoint & pos);

private:
	spell_checker_t * checker_ = nullptr;
	QSyntaxHighlighter * highlighter_ = nullptr;

	std::string get_word_at_cursor(editor_text_edit_t * editor, const QPoint & pos, int & start, int & end) const;
};
