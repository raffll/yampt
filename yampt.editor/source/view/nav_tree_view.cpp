#include "nav_tree_view.hpp"
#include <functional>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTreeView>
#include <QVBoxLayout>

nav_tree_view_t::nav_tree_view_t(plugin_scan_t & scan, QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_tree = new QTreeView(this);
	m_tree->setDragEnabled(false);
	m_tree->setAcceptDrops(false);
	m_tree->setSortingEnabled(true);
	m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
	layout->addWidget(m_tree);

	m_model = new nav_tree_model_t(scan, this);
	m_tree->setModel(m_model);

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

	connect(
	    m_tree,
	    &QTreeView::customContextMenuRequested,
	    this,
	    [this](const QPoint & pos)
	{
		const auto & index = m_tree->indexAt(pos);
		if (!index.isValid())
			return;

		const auto & info = m_model->node_at(index);
		if (info.plugin_idx < 0)
			return;

		emit context_menu_requested(m_tree->viewport()->mapToGlobal(pos), info);
	});

	m_tree->viewport()->installEventFilter(this);
}

void nav_tree_view_t::rebuild()
{
	m_model->rebuild();
	m_tree->setColumnWidth(0, 280);
}

void nav_tree_view_t::rebuild_preserving_state()
{
	save_expansion_state();
	m_model->rebuild();
	restore_expansion_state();
}

void nav_tree_view_t::set_filter(const nav_tree_model_t::filter_state_t & state)
{
	m_model->set_filter(state);
}

void nav_tree_view_t::clear_filter()
{
	m_model->clear_filter();
}

void nav_tree_view_t::set_hide_duplicates(bool hide)
{
	m_model->set_hide_duplicates(hide);
}

void nav_tree_view_t::set_excluded_plugins(const std::set<std::string> * excluded)
{
	m_model->set_excluded_plugins(excluded);
}

void nav_tree_view_t::set_patch_plugins(const std::set<std::string> * patch)
{
	m_model->set_patch_plugins(patch);
}

nav_tree_model_t::node_info_t nav_tree_view_t::current_selection() const
{
	const auto & current = m_tree->currentIndex();
	if (!current.isValid())
		return { -1, {}, {} };

	return m_model->node_at(current);
}

nav_tree_model_t::node_info_t nav_tree_view_t::node_at(const QModelIndex & index) const
{
	if (!index.isValid())
		return { -1, {}, {} };

	return m_model->node_at(index);
}

QModelIndex nav_tree_view_t::find_index(const std::string & rec_type, const std::string & record_id) const
{
	return m_model->find_index(rec_type, record_id);
}

QModelIndex nav_tree_view_t::parent_index(const QModelIndex & index) const
{
	return m_model->parent(index);
}

QTreeView * nav_tree_view_t::tree_widget() const
{
	return m_tree;
}

void nav_tree_view_t::save_expansion_state()
{
	m_expanded_items.clear();

	std::function<void(const QModelIndex &, const std::string &)> collect =
	    [&](const QModelIndex & parent, const std::string & path)
	{
		const auto rows = m_model->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto & idx = m_model->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			const auto & text = m_model->data(idx, Qt::DisplayRole).toString().toStdString();
			const auto & full_path = path + "/" + text;

			if (!m_tree->isExpanded(idx))
				continue;

			m_expanded_items.insert(full_path);
			collect(idx, full_path);
		}
	};

	collect(QModelIndex(), "");
}

void nav_tree_view_t::restore_expansion_state()
{
	std::function<void(const QModelIndex &, const std::string &)> restore =
	    [&](const QModelIndex & parent, const std::string & path)
	{
		const auto rows = m_model->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto & idx = m_model->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			const auto & text = m_model->data(idx, Qt::DisplayRole).toString().toStdString();
			const auto & full_path = path + "/" + text;

			if (!m_expanded_items.count(full_path))
				continue;

			m_tree->expand(idx);
			restore(idx, full_path);
		}
	};

	restore(QModelIndex(), "");
}

bool nav_tree_view_t::eventFilter(QObject * obj, QEvent * event)
{
	if (obj != m_tree->viewport())
		return QWidget::eventFilter(obj, event);

	if (event->type() == QEvent::DragEnter)
	{
		auto * drag = static_cast<QDragEnterEvent *>(event);
		if (drag->mimeData()->hasFormat("application/x-yampt-record"))
		{
			drag->acceptProposedAction();
			return true;
		}
	}

	if (event->type() == QEvent::DragMove)
	{
		auto * drag = static_cast<QDragMoveEvent *>(event);
		if (!drag->mimeData()->hasFormat("application/x-yampt-record"))
		{
			drag->ignore();
			return true;
		}

		const auto & target = m_tree->indexAt(drag->position().toPoint());
		if (target.isValid())
		{
			drag->acceptProposedAction();
			return true;
		}

		drag->ignore();
		return true;
	}

	return QWidget::eventFilter(obj, event);
}
