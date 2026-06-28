#include "dict_selection_dialog.hpp"
#include "../utility/display_name.hpp"
#include "../model/sidebar_model.hpp"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

static std::string normalize_path(std::string input)
{
	std::replace(input.begin(), input.end(), '\\', '/');
	std::transform(
	    input.begin(), input.end(), input.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return input;
}

dict_selection_dialog_t::dict_selection_dialog_t(
    const std::vector<dict_entry_t> & entries,
    const std::vector<std::string> & saved_order,
    QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle("Select Dictionaries");
	setModal(true);
	resize(450, 500);

	auto * layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel("Available dictionaries:", this));

	tree_ = new QTreeWidget(this);
	tree_->setHeaderHidden(true);
	tree_->setRootIsDecorated(true);
	tree_->setIndentation(16);
	layout->addWidget(tree_);

	order_list_ = new QListWidget(this);

	populate_tree(entries);
	populate_order_list(entries, saved_order);

	layout->addWidget(new QLabel("Merge order (last wins):", this));
	layout->addWidget(order_list_);

	setup_buttons(layout);
	connect_signals();
	update_ok_button();
}

void dict_selection_dialog_t::populate_tree(const std::vector<dict_entry_t> & entries)
{
	std::map<std::string, root_content_t> roots;

	for (const auto & entry : entries)
	{
		if (entry.root_path.empty())
			continue;

		if (entry.subfolder.empty())
			roots[entry.root_path].root_items.push_back(&entry);
		else
			roots[entry.root_path].subfolder_items[entry.subfolder].push_back(&entry);
	}

	for (const auto & [root_path, content] : roots)
		add_tree_root_node(root_path, content);
}

void dict_selection_dialog_t::add_tree_root_node(const std::string & root_path, const root_content_t & content)
{
	auto root_label = root_path;
	const auto & separator = root_label.find_last_of("/\\");
	if (separator != std::string::npos)
		root_label = root_label.substr(separator + 1);

	if (root_label == "workspace")
		root_label = workspace_label;

	auto * root_node = new QTreeWidgetItem(tree_);
	root_node->setText(0, QString::fromStdString(root_label));
	root_node->setFlags(Qt::ItemIsEnabled);
	QFont bold_font = root_node->font(0);
	bold_font.setBold(true);
	root_node->setFont(0, bold_font);

	add_dict_tree_items(root_node, content.root_items);

	for (const auto & [subfolder, sub_items] : content.subfolder_items)
	{
		auto * folder_node = new QTreeWidgetItem(root_node);
		folder_node->setText(0, QString::fromStdString(subfolder));
		folder_node->setFlags(Qt::ItemIsEnabled);
		folder_node->setForeground(0, QColor(130, 130, 130));

		add_dict_tree_items(folder_node, sub_items);
		folder_node->setExpanded(true);
	}

	root_node->setExpanded(true);
}

void dict_selection_dialog_t::add_dict_tree_items(
    QTreeWidgetItem * parent_item,
    const std::vector<const dict_entry_t *> & items)
{
	for (const auto * entry : items)
	{
		auto * item = new QTreeWidgetItem(parent_item);
		display_name_t dname(entry->name);
		dname.set_kind(entry->kind);
		item->setText(0, QString::fromStdString(dname.to_string()));
		item->setData(0, Qt::UserRole, QString::fromStdString(entry->path));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(0, entry->checked ? Qt::Checked : Qt::Unchecked);

		if (entry->kind == dict_kind_t::base)
			item->setForeground(0, QColor(180, 140, 80));
		else
			item->setForeground(0, QColor(100, 160, 220));
	}
}

void dict_selection_dialog_t::populate_order_list(
    const std::vector<dict_entry_t> & entries,
    const std::vector<std::string> & saved_order)
{
	if (!saved_order.empty())
	{
		for (const auto & saved_path : saved_order)
		{
			const auto & normalized = normalize_path(saved_path);
			for (const auto & entry : entries)
			{
				if (normalize_path(entry.path) != normalized)
					continue;

				display_name_t dname(entry.name);
				dname.set_kind(entry.kind);
				auto * order_item = new QListWidgetItem(QString::fromStdString(dname.to_string()));
				order_item->setData(Qt::UserRole, QString::fromStdString(entry.path));
				order_list_->addItem(order_item);
				break;
			}
		}

		return;
	}

	for (const auto & entry : entries)
	{
		if (!entry.checked)
			continue;

		display_name_t dname(entry.name);
		dname.set_kind(entry.kind);
		auto * order_item = new QListWidgetItem(QString::fromStdString(dname.to_string()));
		order_item->setData(Qt::UserRole, QString::fromStdString(entry.path));
		order_list_->addItem(order_item);
	}
}

void dict_selection_dialog_t::setup_buttons(QVBoxLayout * layout)
{
	auto * order_buttons = new QHBoxLayout;
	up_button_ = new QPushButton("Up", this);
	down_button_ = new QPushButton("Down", this);
	order_buttons->addWidget(up_button_);
	order_buttons->addWidget(down_button_);
	order_buttons->addStretch();
	layout->addLayout(order_buttons);

	button_box_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(button_box_);
}

void dict_selection_dialog_t::connect_signals()
{
	connect(tree_, &QTreeWidget::itemChanged, this, &dict_selection_dialog_t::on_tree_item_changed);
	connect(up_button_, &QPushButton::clicked, this, &dict_selection_dialog_t::on_move_up);
	connect(down_button_, &QPushButton::clicked, this, &dict_selection_dialog_t::on_move_down);
	connect(button_box_, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box_, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

std::vector<std::string> dict_selection_dialog_t::get_selected_paths() const
{
	std::vector<std::string> result;
	for (int index = 0; index < order_list_->count(); ++index)
		result.push_back(order_list_->item(index)->data(Qt::UserRole).toString().toStdString());

	return result;
}

void dict_selection_dialog_t::on_tree_item_changed(QTreeWidgetItem * item, int)
{
	const auto & path_data = item->data(0, Qt::UserRole).toString();
	if (path_data.isEmpty())
		return;

	if (item->checkState(0) == Qt::Checked)
	{
		for (int index = 0; index < order_list_->count(); ++index)
		{
			if (order_list_->item(index)->data(Qt::UserRole).toString() == path_data)
				return;
		}

		auto * order_item = new QListWidgetItem(item->text(0));
		order_item->setData(Qt::UserRole, path_data);
		order_list_->addItem(order_item);
	}
	else
	{
		for (int index = 0; index < order_list_->count(); ++index)
		{
			if (order_list_->item(index)->data(Qt::UserRole).toString() == path_data)
			{
				delete order_list_->takeItem(index);
				break;
			}
		}
	}

	update_ok_button();
}

void dict_selection_dialog_t::on_move_up()
{
	int current_row = order_list_->currentRow();
	if (current_row <= 0)
		return;

	auto * item = order_list_->takeItem(current_row);
	order_list_->insertItem(current_row - 1, item);
	order_list_->setCurrentRow(current_row - 1);
}

void dict_selection_dialog_t::on_move_down()
{
	int current_row = order_list_->currentRow();
	if (current_row < 0 || current_row >= order_list_->count() - 1)
		return;

	auto * item = order_list_->takeItem(current_row);
	order_list_->insertItem(current_row + 1, item);
	order_list_->setCurrentRow(current_row + 1);
}

void dict_selection_dialog_t::update_ok_button()
{
	button_box_->button(QDialogButtonBox::Ok)->setEnabled(order_list_->count() > 0);
}
