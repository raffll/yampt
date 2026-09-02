#include "highlight_applier.hpp"
#include "../view/translation_edit_view.hpp"
#include "highlight_coordinator.hpp"
#include <utility/string_utils.hpp>
#include <theme_system.hpp>
#include <QPlainTextEdit>
#include <QString>
#include <QTextCursor>

QList<QTextEdit::ExtraSelection> highlight_applier_t::build_selections(
    translation_edit_view_t * editor,
    const std::vector<highlight_position_t> & highlights,
    const std::set<highlight_kind_t> & enabled_kinds)
{
	QList<QTextEdit::ExtraSelection> selections;
	auto * plain_edit = static_cast<QPlainTextEdit *>(editor);
	const auto text_utf8 = string_utils::to_lower_utf8(plain_edit->toPlainText().toStdString());

	for (const auto & highlight : highlights)
	{
		if (enabled_kinds.find(highlight.kind) == enabled_kinds.end())
			continue;

		QTextEdit::ExtraSelection sel;

		const bool dark = theme_system_t::instance().active_theme() == theme_t::dark;
		switch (highlight.kind)
		{
		case highlight_kind_t::hyperlink:
			sel.format.setBackground(dark ? QColor(40, 55, 75) : QColor(200, 220, 255));
			break;
		case highlight_kind_t::inflection:
			sel.format.setBackground(dark ? QColor(60, 45, 70) : QColor(210, 185, 235));
			break;
		case highlight_kind_t::glossary:
			sel.format.setBackground(dark ? QColor(35, 60, 40) : QColor(200, 240, 200));
			break;
		}

		const int char_start = string_utils::utf8_byte_to_char_offset(text_utf8, highlight.start);
		const int char_end = string_utils::utf8_byte_to_char_offset(text_utf8, highlight.start + highlight.length);

		sel.cursor = QTextCursor(plain_edit->document());
		sel.cursor.setPosition(char_start);
		sel.cursor.setPosition(char_end, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

	return selections;
}

void highlight_applier_t::apply(translation_edit_view_t * editor, const extra_selections_state_t & state)
{
	QList<QTextEdit::ExtraSelection> merged;
	merged.append(state.annotations);
	merged.append(state.grammar);
	merged.append(state.adapted_diff);
	merged.append(state.overflow);

	auto * plain_edit = static_cast<QPlainTextEdit *>(editor);
	plain_edit->setExtraSelections(merged);
}
