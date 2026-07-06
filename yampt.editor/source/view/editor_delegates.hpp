#pragma once

#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyleOptionHeader>

class grid_delegate_t : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override
	{
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);

		auto bg = index.data(Qt::BackgroundRole);
		if (bg.isValid())
		{
			painter->fillRect(opt.rect, bg.value<QBrush>());
			opt.backgroundBrush = bg.value<QBrush>();
		}

		QStyledItemDelegate::paint(painter, opt, index);
	}
};

class colored_header_t : public QHeaderView
{
public:
	using QHeaderView::QHeaderView;

protected:
	void paintSection(QPainter * painter, const QRect & rect, int section) const override
	{
		auto fg = model()->headerData(section, Qt::Horizontal, Qt::ForegroundRole);
		if (!fg.isValid())
		{
			QHeaderView::paintSection(painter, rect, section);
			return;
		}

		painter->save();

		QStyleOptionHeader opt;
		initStyleOption(&opt);
		opt.rect = rect;
		opt.section = section;
		opt.text = {};
		style()->drawControl(QStyle::CE_Header, &opt, painter, this);

		auto text = model()->headerData(section, Qt::Horizontal, Qt::DisplayRole).toString();
		auto text_rect = style()->subElementRect(QStyle::SE_HeaderLabel, &opt, this);
		painter->setPen(fg.value<QBrush>().color());
		painter->drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, text);

		painter->restore();
	}
};
