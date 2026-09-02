#pragma once

#include <vector>
#include <QList>
#include <QTextEdit>

struct highlight_position_t;
class translation_edit_view_t;

struct extra_selections_state_t
{
	QList<QTextEdit::ExtraSelection> annotations;
	QList<QTextEdit::ExtraSelection> grammar;
	QList<QTextEdit::ExtraSelection> adapted_diff;
	QList<QTextEdit::ExtraSelection> overflow;
};

#include "highlight_coordinator.hpp"
#include <set>

class highlight_applier_t
{
public:
	static QList<QTextEdit::ExtraSelection> build_selections(
	    translation_edit_view_t * editor,
	    const std::vector<highlight_position_t> & highlights,
	    const std::set<highlight_kind_t> & enabled_kinds);

	static void apply(translation_edit_view_t * editor, const extra_selections_state_t & state);
};
