#include "editor_text_edit.hpp"

#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QPaintEvent>
#include <QTextBlock>

editor_text_edit_t::editor_text_edit_t(QWidget * parent)
	: QPlainTextEdit(parent)
{
}

void editor_text_edit_t::set_block_multiline(bool value)
{
	block_multiline_ = value;
}

void editor_text_edit_t::set_auto_capitalize(bool value)
{
	auto_capitalize_ = value;
}

void editor_text_edit_t::set_show_whitespace(bool value)
{
	show_whitespace_ = value;
}

void editor_text_edit_t::set_record_type(tools_t::rec_type_t type)
{
	record_type_ = type;
}

void editor_text_edit_t::keyPressEvent(QKeyEvent * event)
{
	if (event->modifiers() == Qt::ShiftModifier && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter))
	{
		if (block_multiline_)
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
				return;
			}
		}

		emit navigate_next();
		return;
	}

	if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Down)
	{
		emit navigate_next();
		return;
	}

	if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Up)
	{
		emit navigate_prev();
		return;
	}

	if (block_multiline_ && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter))
	{
		event->ignore();
		return;
	}

	if (block_multiline_ && textCursor().hasSelection())
	{
		auto selected = textCursor().selectedText();
		if (selected.contains(QChar::ParagraphSeparator) || selected.contains('\n'))
		{
			if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete || !event->text().isEmpty())
			{
				event->ignore();
				return;
			}
		}
	}

	if (block_multiline_ && event->key() == Qt::Key_Backspace)
	{
		auto cursor = textCursor();
		if (cursor.atBlockStart() && cursor.position() > 0)
		{
			event->ignore();
			return;
		}
	}

	if (block_multiline_ && event->key() == Qt::Key_Delete)
	{
		auto cursor = textCursor();
		if (cursor.atBlockEnd() && cursor.position() < document()->characterCount() - 1)
		{
			event->ignore();
			return;
		}
	}

	QPlainTextEdit::keyPressEvent(event);

	if (!auto_capitalize_)
		return;

	if (record_type_ == tools_t::rec_type_t::sctx || record_type_ == tools_t::rec_type_t::bnam)
		return;

	if (event->text().length() != 1)
		return;

	if (!event->text()[0].isLetter())
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

void editor_text_edit_t::insertFromMimeData(const QMimeData * source)
{
	if (!block_multiline_)
	{
		QPlainTextEdit::insertFromMimeData(source);
		return;
	}

	auto text = source->text();
	text.replace('\n', ' ');
	text.replace('\r', ' ');
	insertPlainText(text);
}

void editor_text_edit_t::paintEvent(QPaintEvent * event)
{
	QPlainTextEdit::paintEvent(event);

	if (!show_whitespace_)
		return;

	QPainter painter(viewport());
	painter.setPen(QColor(180, 180, 180));
	painter.setFont(document()->defaultFont());

	auto block = firstVisibleBlock();
	while (block.isValid())
	{
		const auto geom = blockBoundingGeometry(block).translated(contentOffset());
		if (geom.top() > event->rect().bottom())
			break;

		if (block.isVisible())
		{
			const auto & text = block.text();
			const auto fm = QFontMetrics(document()->defaultFont());
			const auto text_width = fm.horizontalAdvance(text);
			const auto x = geom.left() + text_width;
			const auto y = geom.top() + fm.ascent();
			painter.drawText(QPointF(x, y), QStringLiteral("\u00B6"));
		}

		block = block.next();
	}
}
