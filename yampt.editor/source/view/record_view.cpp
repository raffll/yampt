#include "record_view.hpp"
#include <scanner/plugin_scan.hpp>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyleOptionHeader>
#include <QTreeView>
#include <QVBoxLayout>

class record_grid_delegate_t : public QStyledItemDelegate
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

class record_colored_header_t : public QHeaderView
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

record_view_t::record_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_model = new view_tree_model_t(this);
	setup_tree();
	layout->addWidget(m_tree);
}

void record_view_t::setup_tree()
{
	m_tree = new QTreeView(this);
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tree->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_tree->setRootIsDecorated(true);
	m_tree->setAlternatingRowColors(false);
	m_tree->setWordWrap(false);
	m_tree->setUniformRowHeights(true);
	m_tree->setDragEnabled(false);
	m_tree->setAcceptDrops(false);
	m_tree->setItemDelegate(new record_grid_delegate_t(m_tree));
	m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tree->setModel(m_model);
	m_tree->setHeader(new record_colored_header_t(Qt::Horizontal, m_tree));
	m_tree->header()->setStretchLastSection(true);
	m_tree->header()->setMinimumSectionSize(120);

	connect(
	    m_tree,
	    &QTreeView::customContextMenuRequested,
	    this,
	    [this](const QPoint & pos)
	{
		auto index = m_tree->indexAt(pos);
		if (!index.isValid())
			return;

		emit context_menu_requested(m_tree->viewport()->mapToGlobal(pos), index);
	});

	connect(
	    m_tree->selectionModel(),
	    &QItemSelectionModel::currentChanged,
	    this,
	    [this](const QModelIndex & current)
	{
		emit selection_changed(current);
	});
}

void record_view_t::display_record(plugin_scan_t & scan, const conflict_entry_t & entry)
{
	m_model->set_record(scan, entry);

	const auto merge_col = m_model->merge_column();
	if (merge_col >= 0)
	{
		const bool should_show = scan.has_merge();
		m_tree->header()->setSectionHidden(merge_col, !should_show);
	}

	expand_non_numeric_groups();
	apply_column_sizing();
}

void record_view_t::clear()
{
	m_model->clear();
}

void record_view_t::resize_columns()
{
	apply_column_sizing();
}

view_tree_model_t * record_view_t::model() const
{
	return m_model;
}

QTreeView * record_view_t::tree() const
{
	return m_tree;
}

void record_view_t::expand_non_numeric_groups()
{
	for (int i = 0; i < m_model->rowCount({}); ++i)
	{
		const auto & idx = m_model->index(i, 0, {});
		if (m_model->rowCount(idx) == 0)
			continue;

		const auto & child = m_model->index(0, 0, idx);
		const auto & child_name = child.data(Qt::DisplayRole).toString();
		if (!child_name.isEmpty() && !child_name[0].isDigit())
			m_tree->expand(idx);
	}
}

void record_view_t::apply_column_sizing()
{
	for (int i = 0; i < m_model->columnCount({}); ++i)
		m_tree->resizeColumnToContents(i);

	const auto total_width = m_tree->viewport()->width();
	const auto col_count = m_model->columnCount({});
	if (col_count <= 1 || total_width <= 0)
		return;

	const auto label_width = std::min(m_tree->columnWidth(0), total_width / 3);
	const auto remaining = total_width - label_width;
	const auto per_col = remaining / (col_count - 1);

	m_tree->setColumnWidth(0, label_width);
	for (int i = 1; i < col_count; ++i)
		m_tree->setColumnWidth(i, per_col);
}
