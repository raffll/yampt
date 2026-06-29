#include "line_number_gutter.hpp"
#include "translation_edit_view.hpp"

#include <QFontMetrics>
#include <QPainter>
#include <QTextBlock>

line_number_gutter_t::line_number_gutter_t(translation_edit_view_t * editor, QWidget * parent)
    : QWidget(parent)
    , editor_(editor)
{
	setFixedWidth(calculate_width());

	connect(editor_, &translation_edit_view_t::blockCountChanged, this, [this]() { setFixedWidth(calculate_width()); });

	connect(
	    editor_,
	    &translation_edit_view_t::updateRequest,
	    this,
	    [this](const QRect & rect, int dy)
	{
		if (dy != 0)
			scroll(0, dy);
		else
			update(0, rect.y(), width(), rect.height());
	});
}

QSize line_number_gutter_t::sizeHint() const
{
	return QSize(calculate_width(), 0);
}

void line_number_gutter_t::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	painter.fillRect(event->rect(), QColor(230, 230, 230));

	auto block = editor_->firstVisibleBlock();
	int top = static_cast<int>(editor_->blockBoundingGeometry(block).translated(editor_->contentOffset()).top());
	int bottom = top + static_cast<int>(editor_->blockBoundingRect(block).height());

	while (block.isValid() && top <= event->rect().bottom())
	{
		if (block.isVisible() && bottom >= event->rect().top())
		{
			const auto number = QString::number(block.blockNumber() + 1);
			painter.setPen(QColor(100, 100, 100));
			painter.drawText(0, top, width() - 4, fontMetrics().height(), Qt::AlignRight, number);
		}

		block = block.next();
		top = bottom;
		bottom = top + static_cast<int>(editor_->blockBoundingRect(block).height());
	}
}

int line_number_gutter_t::calculate_width() const
{
	int digits = 1;
	int max = editor_->blockCount();
	while (max >= 10)
	{
		max /= 10;
		++digits;
	}

	return fontMetrics().horizontalAdvance('9') * digits + 12;
}
