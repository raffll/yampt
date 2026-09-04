#include "lua_tree_view.hpp"
#include <QTreeView>
#include <QVBoxLayout>

lua_tree_view_t::lua_tree_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_tree = new QTreeView(this);
	m_tree->setRootIsDecorated(true);
	layout->addWidget(m_tree);

	m_model = new lua_tree_model_t(this);
	m_tree->setModel(m_model);

	const int handler_column_width = m_tree->fontMetrics().horizontalAdvance(QString(40, '0')) + m_tree->indentation();
	m_tree->setColumnWidth(0, handler_column_width);

	connect(
	    m_tree->selectionModel(),
	    &QItemSelectionModel::currentChanged,
	    this,
	    [this](const QModelIndex & current)
	{
		if (!current.isValid())
			return;

		const auto & info = m_model->node_at(current);
		emit selection_changed(info);
	});
}

void lua_tree_view_t::set_scan_result(const lua_scan_result_t & result)
{
	m_model->set_scan_result(result);
	m_tree->collapseAll();
}

void lua_tree_view_t::clear()
{
	m_model->clear();
}

lua_tree_model_t::node_info_t lua_tree_view_t::current_selection() const
{
	const auto & current = m_tree->currentIndex();
	if (!current.isValid())
		return {};

	return m_model->node_at(current);
}

const lua_scan_result_t & lua_tree_view_t::scan_result() const
{
	return m_model->scan_result();
}
