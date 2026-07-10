#include "merge_dialog.hpp"
#include "../view/display_name.hpp"
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

merge_dialog_t::merge_dialog_t(const std::vector<dict_entry_t> & loaded_dicts, QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Merge Dictionaries"));
	setModal(true);
	resize(450, 400);

	auto * layout = new QVBoxLayout(this);

	layout->addWidget(new QLabel(
	    "Drag or use buttons to set priority order.\n"
	    "Last item has highest priority (last-listed wins).",
	    this));

	m_list = new QListWidget(this);
	m_list->setDragDropMode(QAbstractItemView::InternalMove);
	m_list->setDefaultDropAction(Qt::MoveAction);
	m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	layout->addWidget(m_list);

	populate_list(loaded_dicts);
	setup_buttons();
	connect_signals();
	update_ok_button();
}

void merge_dialog_t::populate_list(const std::vector<dict_entry_t> & loaded_dicts)
{
	for (const auto & entry : loaded_dicts)
	{
		display_name_t dname(entry.name);
		dname.set_kind(entry.kind);

		auto * item = new QListWidgetItem(QString::fromStdString(dname.to_string()));
		item->setData(Qt::UserRole, QString::fromStdString(entry.path));
		item->setCheckState(Qt::Checked);

		if (entry.kind == dict_kind_t::base)
			item->setForeground(QColor(180, 140, 80));
		else
			item->setForeground(QColor(100, 160, 220));

		m_list->addItem(item);
	}
}

void merge_dialog_t::setup_buttons()
{
	auto * layout = qobject_cast<QVBoxLayout *>(this->layout());

	auto * order_buttons = new QHBoxLayout;

	m_up_button = new QPushButton(tr("Up"), this);
	m_up_button->setToolTip(tr("Move selected item up (lower priority)"));

	m_down_button = new QPushButton(tr("Down"), this);
	m_down_button->setToolTip(tr("Move selected item down (higher priority)"));

	m_remove_button = new QPushButton(tr("Remove"), this);
	m_remove_button->setToolTip(tr("Remove selected items from merge"));

	order_buttons->addWidget(m_up_button);
	order_buttons->addWidget(m_down_button);
	order_buttons->addWidget(m_remove_button);
	order_buttons->addStretch();
	layout->addLayout(order_buttons);

	m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	m_button_box->button(QDialogButtonBox::Ok)->setText(tr("Merge"));
	m_button_box->button(QDialogButtonBox::Ok)->setToolTip(tr("Execute merge with listed order"));
	layout->addWidget(m_button_box);
}

void merge_dialog_t::connect_signals()
{
	connect(m_up_button, &QPushButton::clicked, this, &merge_dialog_t::on_move_up);
	connect(m_down_button, &QPushButton::clicked, this, &merge_dialog_t::on_move_down);
	connect(m_remove_button, &QPushButton::clicked, this, &merge_dialog_t::on_remove);
	connect(m_button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_list->model(), &QAbstractItemModel::rowsMoved, this, &merge_dialog_t::update_ok_button);
}

std::vector<std::string> merge_dialog_t::selected_paths() const
{
	std::vector<std::string> result;
	for (int index = 0; index < m_list->count(); ++index)
	{
		const auto * item = m_list->item(index);
		if (item->checkState() == Qt::Checked)
			result.push_back(item->data(Qt::UserRole).toString().toStdString());
	}

	return result;
}

void merge_dialog_t::on_move_up()
{
	int current_row = m_list->currentRow();
	if (current_row <= 0)
		return;

	auto * item = m_list->takeItem(current_row);
	m_list->insertItem(current_row - 1, item);
	m_list->setCurrentRow(current_row - 1);
}

void merge_dialog_t::on_move_down()
{
	int current_row = m_list->currentRow();
	if (current_row < 0 || current_row >= m_list->count() - 1)
		return;

	auto * item = m_list->takeItem(current_row);
	m_list->insertItem(current_row + 1, item);
	m_list->setCurrentRow(current_row + 1);
}

void merge_dialog_t::on_remove()
{
	const auto selected = m_list->selectedItems();
	for (auto * item : selected)
		delete m_list->takeItem(m_list->row(item));

	update_ok_button();
}

void merge_dialog_t::update_ok_button()
{
	int checked_count = 0;
	for (int index = 0; index < m_list->count(); ++index)
	{
		if (m_list->item(index)->checkState() == Qt::Checked)
			++checked_count;
	}

	m_button_box->button(QDialogButtonBox::Ok)->setEnabled(checked_count >= 2);
}
