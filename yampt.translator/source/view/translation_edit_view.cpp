#include "translation_edit_view.hpp"
#include <QKeyEvent>
#include <QMimeData>

translation_edit_view_t::translation_edit_view_t(QWidget * parent)
    : QPlainTextEdit(parent)
{}

void translation_edit_view_t::set_block_multiline(bool value)
{
	m_block_multiline = value;
}

void translation_edit_view_t::set_auto_capitalize(bool value)
{
	m_auto_capitalize = value;
}

void translation_edit_view_t::set_record_type(rec_type_t type)
{
	m_record_type = type;
}

bool translation_edit_view_t::handle_navigation_keys(QKeyEvent * event)
{
	if (event->modifiers() == Qt::ShiftModifier && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter))
	{
		if (m_block_multiline)
		{
			auto cursor = textCursor();
			int current_block = cursor.blockNumber();
			int total_blocks = document()->blockCount();

			if (current_block < total_blocks - 1)
			{
				cursor.movePosition(QTextCursor::Down);
				cursor.movePosition(QTextCursor::StartOfBlock);
				cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
				setTextCursor(cursor);
				return true;
			}
		}

		emit navigate_next();
		return true;
	}

	if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Down)
	{
		emit navigate_next();
		return true;
	}

	if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Up)
	{
		emit navigate_prev();
		return true;
	}

	return false;
}

bool translation_edit_view_t::handle_multiline_guard(QKeyEvent * event)
{
	if (!m_block_multiline)
		return false;

	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
		return true;

	if (textCursor().hasSelection())
	{
		auto selected = textCursor().selectedText();
		if (selected.contains(QChar::ParagraphSeparator) || selected.contains('\n'))
		{
			if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete || !event->text().isEmpty())
				return true;
		}
	}

	if (event->key() == Qt::Key_Backspace)
	{
		auto cursor = textCursor();
		if (cursor.atBlockStart() && cursor.position() > 0)
			return true;
	}

	if (event->key() == Qt::Key_Delete)
	{
		auto cursor = textCursor();
		if (cursor.atBlockEnd() && cursor.position() < document()->characterCount() - 1)
			return true;
	}

	return false;
}

void translation_edit_view_t::apply_auto_capitalize()
{
	if (!m_auto_capitalize)
		return;

	if (m_record_type == rec_type_t::sctx || m_record_type == rec_type_t::bnam)
		return;

	auto cursor = textCursor();
	const auto pos = cursor.position();
	if (pos < 3)
		return;

	const auto & doc = document()->toPlainText();
	const auto before_space = doc[pos - 3];
	const auto space = doc[pos - 2];
	const auto letter = doc[pos - 1];

	if (space != ' ')
		return;

	if (before_space != '.' && before_space != '?' && before_space != '!')
		return;

	if (!letter.isLetter())
		return;

	cursor.setPosition(pos - 1);
	cursor.setPosition(pos, QTextCursor::KeepAnchor);
	cursor.insertText(letter.toUpper());
}

void translation_edit_view_t::keyPressEvent(QKeyEvent * event)
{
	if (handle_navigation_keys(event))
		return;

	if (handle_multiline_guard(event))
	{
		event->ignore();
		return;
	}

	QPlainTextEdit::keyPressEvent(event);

	if (event->text().length() != 1)
		return;

	if (!event->text()[0].isLetter())
		return;

	apply_auto_capitalize();
}

void translation_edit_view_t::insertFromMimeData(const QMimeData * source)
{
	if (!m_block_multiline)
	{
		QPlainTextEdit::insertFromMimeData(source);
		return;
	}

	auto text = source->text();
	text.replace('\n', ' ');
	text.replace('\r', ' ');
	insertPlainText(text);
}
