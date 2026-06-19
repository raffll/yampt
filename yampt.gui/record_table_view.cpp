#include "record_table_view.hpp"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

#include "record_table_model.hpp"

record_table_view_t::record_table_view_t(QWidget * parent)
    : QTableView(parent)
{
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSortingEnabled(true);
	setAlternatingRowColors(false);
	verticalHeader()->setVisible(false);

	auto * header = horizontalHeader();
	header->setStretchLastSection(false);
}

void record_table_view_t::setModel(QAbstractItemModel * model)
{
	QTableView::setModel(model);

	if (!model)
		return;

	auto * header = horizontalHeader();
	if (header->count() >= col_count)
	{
		header->setSectionResizeMode(col_id, QHeaderView::Interactive);
		header->setSectionResizeMode(col_key, QHeaderView::Interactive);
		header->setSectionResizeMode(col_original, QHeaderView::Stretch);
		header->setSectionResizeMode(col_translation, QHeaderView::Stretch);
		header->setSectionResizeMode(col_status, QHeaderView::Interactive);

		header->resizeSection(col_id, 50);
		header->resizeSection(col_key, 200);
		header->resizeSection(col_status, 80);
	}

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

void record_table_view_t::contextMenuEvent(QContextMenuEvent * event)
{
	const auto selected = selectionModel()->selectedRows();
	if (selected.isEmpty())
		return;

	auto * menu = new QMenu(this);

	auto * act_translated = menu->addAction("Set Translated");
	auto * act_in_progress = menu->addAction("Set In Progress");
	auto * act_untranslated = menu->addAction("Set Untranslated");
	auto * act_error = menu->addAction("Set Error");

	auto * chosen = menu->exec(event->globalPos());
	QString new_status;
	if (chosen == act_translated)
		new_status = "translated";
	else if (chosen == act_in_progress)
		new_status = "in_progress";
	else if (chosen == act_untranslated)
		new_status = "untranslated";
	else if (chosen == act_error)
		new_status = "error";

	if (!new_status.isEmpty())
	{
		QList<int> rows;
		for (const auto & idx : selected)
			rows.append(idx.row());

		emit batch_status_change_requested(rows, new_status);
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
