#include "view/lua_detail_view.hpp"
#include "model/lua_detail_model.hpp"
#include <QHeaderView>
#include <QResizeEvent>
#include <QTreeView>
#include <QVBoxLayout>

lua_detail_view_t::lua_detail_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_model = new lua_detail_model_t(this);
	setup_tree();
	layout->addWidget(m_tree);
}

void lua_detail_view_t::show_conflict(const handler_conflict_t & conflict)
{
	m_model->set_conflict(conflict);
	apply_column_sizing();
}

void lua_detail_view_t::show_registration(const handler_registration_t & registration)
{
	m_model->set_registration(registration);
	apply_column_sizing();
}

void lua_detail_view_t::clear()
{
	m_model->clear();
}

void lua_detail_view_t::setup_tree()
{
	m_tree = new QTreeView(this);
	m_tree->setModel(m_model);
	m_tree->setRootIsDecorated(false);
	m_tree->setAlternatingRowColors(true);
	m_tree->setWordWrap(true);
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tree->header()->setStretchLastSection(true);
	m_tree->header()->setMinimumSectionSize(100);
}

void lua_detail_view_t::apply_column_sizing()
{
	const auto col_count = m_model->columnCount({});
	if (col_count <= 1)
		return;

	for (int index = 0; index < col_count; ++index)
		m_tree->resizeColumnToContents(index);

	const auto total_width = m_tree->viewport()->width();
	if (total_width <= 0)
		return;

	const auto label_width = std::min(m_tree->columnWidth(0), total_width / 4);
	const auto remaining = total_width - label_width;
	const auto per_column = remaining / (col_count - 1);

	m_tree->setColumnWidth(0, label_width);
	for (int index = 1; index < col_count; ++index)
		m_tree->setColumnWidth(index, per_column);
}

void lua_detail_view_t::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);

	if (m_model->columnCount({}) > 1)
		apply_column_sizing();
}
