#pragma once

#include <string>

class translation_edit_view_t;
class QPoint;
class QSyntaxHighlighter;
class spell_checker_t;

class spell_context_menu_t
{
public:
	spell_context_menu_t(spell_checker_t * checker, QSyntaxHighlighter * highlighter);

	void show_menu(translation_edit_view_t * editor, const QPoint & pos);

private:
	spell_checker_t * m_checker = nullptr;
	QSyntaxHighlighter * m_highlighter = nullptr;

	std::string get_word_at_cursor(translation_edit_view_t * editor, const QPoint & pos, int & start, int & end) const;
};
