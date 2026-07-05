#include "highlight_applier.hpp"
#include "editor/highlight_coordinator.hpp"
#include "translation_edit_view.hpp"
#include <theme_system.hpp>
#include <QPlainTextEdit>
#include <QTextCursor>

QList<QTextEdit::ExtraSelection> highlight_applier_t::build_selections(
    translation_edit_view_t * editor,
    const std::vector<highlight_position_t> & highlights)
{
	QList<QTextEdit::ExtraSelection> selections;

	for (const auto & highlight : highlights)
	{
		QTextEdit::ExtraSelection sel;

		if (theme_system_t::instance().active_theme() == theme_t::dark)
			sel.format.setBackground(highlight.is_hyperlink ? QColor(40, 55, 75) : QColor(35, 60, 40));
		else
			sel.format.setBackground(highlight.is_hyperlink ? QColor(200, 220, 255) : QColor(200, 240, 200));

		auto * plain_edit = static_cast<QPlainTextEdit *>(editor);
		sel.cursor = QTextCursor(plain_edit->document());
		sel.cursor.setPosition(highlight.start);
		sel.cursor.setPosition(highlight.start + highlight.length, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

	return selections;
}

void highlight_applier_t::apply(
    translation_edit_view_t * editor,
    const extra_selections_state_t & state)
{
	QList<QTextEdit::ExtraSelection> merged;
	merged.append(state.annotations);
	merged.append(state.grammar);
	merged.append(state.adapted_diff);

	auto * plain_edit = static_cast<QPlainTextEdit *>(editor);
	plain_edit->setExtraSelections(merged);
}
