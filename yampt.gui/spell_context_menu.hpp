#pragma once

#include <string>

class QTextEdit;
class QPoint;
class spell_checker_t;
class spell_check_highlighter_t;

class spell_context_menu_t
{
public:
	spell_context_menu_t(spell_checker_t * checker, spell_check_highlighter_t * highlighter);

	void show_menu(QTextEdit * editor, const QPoint & pos);

private:
	spell_checker_t * checker_ = nullptr;
	spell_check_highlighter_t * highlighter_ = nullptr;

	std::string get_word_at_cursor(QTextEdit * editor, const QPoint & pos, int & start, int & end) const;
};
