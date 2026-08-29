#include "record_table_view.hpp"
#include "../model/record_table_model.hpp"
#include <translation_example.hpp>
#include <optional>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>

record_table_view_t::record_table_view_t(QWidget * parent)
    : QTableView(parent)
{
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSortingEnabled(true);
	setAlternatingRowColors(false);
	setEditTriggers(QAbstractItemView::DoubleClicked);
	verticalHeader()->setVisible(false);
	verticalHeader()->setDefaultSectionSize(20);
	verticalHeader()->setMinimumSectionSize(20);

	auto * header = horizontalHeader();
	header->setStretchLastSection(false);
}

void record_table_view_t::setModel(QAbstractItemModel * model)
{
	QTableView::setModel(model);

	if (!model)
		return;

	apply_column_layout();

	connect(
	    selectionModel(),
	    &QItemSelectionModel::selectionChanged,
	    this,
	    [this]()
	{
		const auto selected = selectionModel()->selectedRows();
		if (selected.count() != 1)
			return;

		emit row_selected(selected.first().row());
	});
}

void record_table_view_t::apply_column_layout()
{
	const auto * record_model = qobject_cast<record_table_model_t *>(model());
	if (!record_model)
		return;

	const auto & columns = record_model->columns();
	auto * header = horizontalHeader();
	header->setStretchLastSection(false);

	for (int position = 0; position < columns.count(); ++position)
	{
		const auto logical = columns.at(position);
		if (logical == col_translation)
		{
			header->setSectionResizeMode(position, QHeaderView::Stretch);
			continue;
		}

		header->setSectionResizeMode(position, QHeaderView::Interactive);
		header->resizeSection(position, default_column_width(logical));
	}
}

int record_table_view_t::default_column_width(table_col_t logical_column)
{
	switch (logical_column)
	{
	case col_id:
		return 50;
	case col_key:
		return 200;
	case col_original:
		return 250;
	case col_status:
		return 90;
	default:
		return 100;
	}
}

void record_table_view_t::refresh_column_layout()
{
	apply_column_layout();
}

void record_table_view_t::set_context_menu_enabled(bool enabled)
{
	m_context_menu_enabled = enabled;
}

void record_table_view_t::set_example_state_fn(std::function<bool(int row)> fn)
{
	m_example_state_fn = std::move(fn);
}

void record_table_view_t::set_example_count_fn(std::function<int()> fn)
{
	m_example_count_fn = std::move(fn);
}

void record_table_view_t::set_can_revert_fn(std::function<bool(int row)> fn)
{
	m_can_revert_fn = std::move(fn);
}

void record_table_view_t::contextMenuEvent(QContextMenuEvent * event)
{
	if (!m_context_menu_enabled)
		return;

	const auto selected = selectionModel()->selectedRows();
	if (selected.isEmpty())
		return;

	auto * menu = new QMenu(this);

	auto * act_translated = menu->addAction(tr("Set Translated"));
	auto * act_in_progress = menu->addAction(tr("Set In Progress"));
	auto * act_untranslated = menu->addAction(tr("Set Untranslated"));
	auto * act_error = menu->addAction(tr("Set Error"));

	menu->addSeparator();
	auto * act_revert = menu->addAction(tr("Revert"));
	act_revert->setToolTip(tr("Revert selected entries to previous state from history"));

	if (m_can_revert_fn)
	{
		bool any_can_revert = false;
		for (const auto & idx : selected)
		{
			if (!m_can_revert_fn(idx.row()))
				continue;

			any_can_revert = true;
			break;
		}

		act_revert->setEnabled(any_can_revert);
	}

	menu->addSeparator();
	const auto first_row = selected.first().row();
	const auto already_example = m_example_state_fn && m_example_state_fn(first_row);
	auto * act_example = menu->addAction(already_example ? tr("Unmark Example") : tr("Mark as Example"));
	act_example->setToolTip(tr("Use this entry as an AI translation style example"));

	if (!already_example && m_example_count_fn && m_example_count_fn() >= max_examples)
		act_example->setEnabled(false);

	auto * chosen = menu->exec(event->globalPos());
	std::optional<status_t> new_status;
	if (chosen == act_translated)
		new_status = status_t::translated;
	else if (chosen == act_in_progress)
		new_status = status_t::in_progress;
	else if (chosen == act_untranslated)
		new_status = status_t::untranslated;
	else if (chosen == act_error)
		new_status = status_t::error;

	if (new_status.has_value())
	{
		QList<int> rows;
		for (const auto & idx : selected)
			rows.append(idx.row());

		emit batch_status_change_requested(rows, new_status.value());
	}
	else if (chosen == act_revert)
	{
		QList<int> rows;
		for (const auto & idx : selected)
			rows.append(idx.row());

		emit batch_revert_requested(rows);
	}
	else if (chosen == act_example)
	{
		QList<int> rows;
		for (const auto & idx : selected)
			rows.append(idx.row());

		emit toggle_example_requested(rows);
	}

	delete menu;
}

void record_table_view_t::set_column_widths(const std::vector<int> & widths)
{
	auto * header = horizontalHeader();
	for (size_t i = 0; i < widths.size(); ++i)
	{
		if (static_cast<int>(i) >= header->count())
			break;

		if (header->sectionResizeMode(static_cast<int>(i)) == QHeaderView::Stretch)
			continue;

		header->resizeSection(static_cast<int>(i), widths[i]);
	}
}

std::vector<int> record_table_view_t::get_column_widths() const
{
	std::vector<int> widths;
	const auto * header = horizontalHeader();
	for (int i = 0; i < header->count(); ++i)
		widths.push_back(header->sectionSize(i));

	return widths;
}

void record_table_view_t::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Delete && !event->modifiers())
	{
		const auto selected = selectionModel()->selectedRows();
		if (!selected.isEmpty())
		{
			emit delete_entry_requested();
			return;
		}
	}

	QTableView::keyPressEvent(event);
}
