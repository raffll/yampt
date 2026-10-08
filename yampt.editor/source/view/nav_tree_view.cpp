#include "nav_tree_view.hpp"
#include "type_ahead.hpp"
#include <utility/string_utils.hpp>
#include <vector>
#include <functional>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMimeData>
#include <QSignalBlocker>
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
	m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tree->setAllColumnsShowFocus(true);
	m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
	layout->addWidget(m_tree);

	m_model = new nav_tree_model_t(scan, this);
	m_tree->setModel(m_model);

	const int id_column_width = m_tree->fontMetrics().horizontalAdvance(QString(30, '0')) + m_tree->indentation();
	m_tree->setColumnWidth(0, id_column_width);

	m_tree->header()->setStretchLastSection(true);
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
	m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);

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
	m_tree->installEventFilter(this);
}

void nav_tree_view_t::rebuild()
{
	m_model->rebuild();
}

void nav_tree_view_t::rebuild_preserving_state()
{
	const auto selected = current_selection();

	save_expansion_state();
	m_model->rebuild();
	restore_expansion_state();

	restore_selection(selected);
}

void nav_tree_view_t::restore_selection(const nav_tree_model_t::node_info_t & info)
{
	if (info.plugin_idx < 0)
		return;

	const auto index = m_model->index_for_node(info);
	if (!index.isValid())
		return;

	const QSignalBlocker blocker(m_tree->selectionModel());
	m_tree->setCurrentIndex(index);
}

void nav_tree_view_t::refresh_colors()
{
	m_model->refresh_colors();
}

void nav_tree_view_t::notify_record_changed(const std::string & rec_type, const std::string & record_id)
{
	m_model->notify_record_changed(rec_type, record_id);
}

void nav_tree_view_t::notify_plugin_changed(int plugin_idx)
{
	m_model->notify_plugin_changed(plugin_idx);
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

void nav_tree_view_t::set_show_deleted_strikeout(bool value)
{
	m_model->set_show_deleted_strikeout(value);
}

void nav_tree_view_t::set_patch_plugins(const std::set<std::string> * patch)
{
	m_model->set_patch_plugins(patch);
}

void nav_tree_view_t::set_editable_columns(const editable_column_set_t * editable)
{
	m_model->set_editable_columns(editable);
}

void nav_tree_view_t::set_display_codepage(codepage_t codepage)
{
	m_model->set_display_codepage(codepage);
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

void nav_tree_view_t::select_record(const std::string & rec_type, const std::string & record_id)
{
	const auto index = m_model->find_index(rec_type, record_id);
	if (!index.isValid())
		return;

	const QSignalBlocker blocker(m_tree->selectionModel());
	m_tree->setCurrentIndex(index);
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

std::string nav_tree_view_t::node_path_key(const QModelIndex & index) const
{
	const auto & info = m_model->node_at(index);
	return std::to_string(info.plugin_idx) + "/" + info.rec_type + "/" + info.record_id;
}

void nav_tree_view_t::save_expansion_state()
{
	m_expanded_items.clear();

	std::function<void(const QModelIndex &)> collect = [&](const QModelIndex & parent)
	{
		const auto rows = m_model->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto & idx = m_model->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			if (!m_tree->isExpanded(idx))
				continue;

			m_expanded_items.insert(node_path_key(idx));
			collect(idx);
		}
	};

	collect(QModelIndex());
}

void nav_tree_view_t::restore_expansion_state()
{
	std::function<void(const QModelIndex &)> restore = [&](const QModelIndex & parent)
	{
		const auto rows = m_model->rowCount(parent);
		for (int i = 0; i < rows; ++i)
		{
			const auto & idx = m_model->index(i, 0, parent);
			if (!idx.isValid())
				continue;

			if (!m_expanded_items.count(node_path_key(idx)))
				continue;

			m_tree->expand(idx);
			restore(idx);
		}
	};

	restore(QModelIndex());
}

bool nav_tree_view_t::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == m_tree && event->type() == QEvent::KeyPress)
	{
		if (handle_type_ahead(static_cast<QKeyEvent *>(event)))
			return true;

		return QWidget::eventFilter(obj, event);
	}

	if (obj == m_tree->viewport())
		return handle_viewport_drag(event);

	return QWidget::eventFilter(obj, event);
}

bool nav_tree_view_t::handle_type_ahead(QKeyEvent * key_event)
{
	const auto modifiers = key_event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
	if (modifiers != Qt::NoModifier)
		return false;

	const auto & text = key_event->text();
	if (text.size() != 1)
		return false;

	const auto character = text.at(0);
	if (!character.isLetterOrNumber())
		return false;

	const auto needle = string_utils::to_lower(std::string(1, character.toLatin1()));
	const auto & current = m_tree->currentIndex();
	const int current_row = (current.isValid() && !current.parent().isValid()) ? current.row() : -1;
	const int matched = next_matching_plugin_row(needle, current_row);

	if (matched < 0)
		return true;

	m_tree->setCurrentIndex(m_model->index(matched, 0, {}));
	m_tree->scrollTo(m_model->index(matched, 0, {}));
	return true;
}

int nav_tree_view_t::next_matching_plugin_row(const std::string & lowered_prefix, int start_row) const
{
	const int total_rows = m_model->rowCount({});

	std::vector<std::string> filenames;
	filenames.reserve(static_cast<size_t>(total_rows));
	for (int row = 0; row < total_rows; ++row)
		filenames.push_back(m_model->plugin_filename_at(row));

	return type_ahead::next_matching_row(filenames, lowered_prefix, start_row);
}

bool nav_tree_view_t::handle_viewport_drag(QEvent * event)
{
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

	return QWidget::eventFilter(m_tree->viewport(), event);
}
