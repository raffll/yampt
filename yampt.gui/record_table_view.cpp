#include "record_table_view.hpp"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

record_table_view_t::record_table_view_t(QWidget * parent)
    : QTableView(parent)
{
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setSortingEnabled(true);
	setAlternatingRowColors(true);
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
	if (header->count() >= 5)
	{
		header->setSectionResizeMode(0, QHeaderView::Interactive);
		header->setSectionResizeMode(1, QHeaderView::Interactive);
		header->setSectionResizeMode(2, QHeaderView::Stretch);
		header->setSectionResizeMode(3, QHeaderView::Stretch);
		header->setSectionResizeMode(4, QHeaderView::Interactive);

		header->resizeSection(0, 50);
		header->resizeSection(1, 150);
		header->resizeSection(4, 80);
	}

	connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
	    [this]()
	    {
		    const auto index = currentIndex();
		    if (index.isValid())
			    emit row_selected(index.row());
	    });
}

void record_table_view_t::contextMenuEvent(QContextMenuEvent * event)
{
	const auto index = indexAt(event->pos());
	if (!index.isValid())
		return;

	const int row = index.row();

	auto * menu = new QMenu(this);
	auto * act_untranslated = menu->addAction("Set Untranslated");
	auto * act_translated = menu->addAction("Set Translated");
	auto * act_error = menu->addAction("Set Error");

	const auto status_index = model()->index(row, 4);
	const auto current_status = model()->data(status_index).toString();
	if (current_status == "error")
	{
		act_untranslated->setEnabled(false);
		act_translated->setEnabled(false);
	}

	auto * selected = menu->exec(event->globalPos());
	if (selected == act_untranslated)
		emit status_change_requested(row, "untranslated");
	else if (selected == act_translated)
		emit status_change_requested(row, "translated");
	else if (selected == act_error)
		emit status_change_requested(row, "error");

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
