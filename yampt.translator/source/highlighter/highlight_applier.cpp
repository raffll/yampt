#include "highlight_applier.hpp"
#include "../view/translation_edit_view.hpp"
#include "highlight_coordinator.hpp"
#include <theme_system.hpp>
#include <QPlainTextEdit>
#include <QString>
#include <QTextCursor>

int highlight_applier_t::utf8_byte_to_char_offset(const std::string & utf8_text, int byte_offset)
{
	int char_count = 0;
	int byte_count = 0;
	const auto * data = reinterpret_cast<const unsigned char *>(utf8_text.data());
	const auto total = static_cast<int>(utf8_text.size());

	while (byte_count < byte_offset && byte_count < total)
	{
		const unsigned char lead = data[byte_count];
		if (lead < 0x80)
			byte_count += 1;
		else if ((lead & 0xE0) == 0xC0)
			byte_count += 2;
		else if ((lead & 0xF0) == 0xE0)
			byte_count += 3;
		else
			byte_count += 4;

		++char_count;
	}

	return char_count;
}

QList<QTextEdit::ExtraSelection> highlight_applier_t::build_selections(
    translation_edit_view_t * editor,
    const std::vector<highlight_position_t> & highlights)
{
	QList<QTextEdit::ExtraSelection> selections;
	auto * plain_edit = static_cast<QPlainTextEdit *>(editor);
	const auto text_utf8 = plain_edit->toPlainText().toLower().toStdString();

	for (const auto & highlight : highlights)
	{
		QTextEdit::ExtraSelection sel;

		if (theme_system_t::instance().active_theme() == theme_t::dark)
			sel.format.setBackground(highlight.is_hyperlink ? QColor(40, 55, 75) : QColor(35, 60, 40));
		else
			sel.format.setBackground(highlight.is_hyperlink ? QColor(200, 220, 255) : QColor(200, 240, 200));

		const int char_start = utf8_byte_to_char_offset(text_utf8, highlight.start);
		const int char_end = utf8_byte_to_char_offset(text_utf8, highlight.start + highlight.length);

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

	auto * plain_edit = static_cast<QPlainTextEdit *>(editor);
	plain_edit->setExtraSelections(merged);
}
