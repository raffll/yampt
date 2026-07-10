#include "dict_selection_dialog.hpp"
#include "../model/sidebar_model.hpp"
#include "../view/display_name.hpp"
#include <utility/string_utils.hpp>
#include <map>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

static std::string normalize_and_lower(std::string_view input)
{
	return string_utils::to_lower(string_utils::normalize_path(input));
}

dict_selection_dialog_t::dict_selection_dialog_t(
    const std::vector<dict_entry_t> & entries,
    const std::vector<std::string> & saved_order,
    QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Select Dictionaries"));
	setModal(true);
	resize(450, 400);

	auto * layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Available dictionaries:"), this));

	m_tree = new QTreeWidget(this);
	m_tree->setHeaderHidden(true);
	m_tree->setRootIsDecorated(true);
	m_tree->setIndentation(16);
	layout->addWidget(m_tree);

	m_order_list = new QListWidget(this);

	populate_tree(entries);
	populate_order_list(entries, saved_order);

	layout->addWidget(new QLabel(tr("Merge order (last wins):"), this));
	layout->addWidget(m_order_list);

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
	auto root_label = std::string(string_utils::extract_filename(root_path));

	if (root_label == "workspace")
		root_label = workspace_label;

	auto * root_node = new QTreeWidgetItem(m_tree);
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
			const auto normalized = string_utils::normalize_path(saved_path);
			for (const auto & entry : entries)
			{
				if (string_utils::normalize_path(entry.path) != normalized)
					continue;

				display_name_t dname(entry.name);
				dname.set_kind(entry.kind);
				auto * order_item = new QListWidgetItem(QString::fromStdString(dname.to_string()));
				order_item->setData(Qt::UserRole, QString::fromStdString(entry.path));
				m_order_list->addItem(order_item);
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
		m_order_list->addItem(order_item);
	}
}

void dict_selection_dialog_t::setup_buttons(QVBoxLayout * layout)
{
	auto * order_buttons = new QHBoxLayout;
	m_up_button = new QPushButton(tr("Up"), this);
	m_down_button = new QPushButton(tr("Down"), this);
	order_buttons->addWidget(m_up_button);
	order_buttons->addWidget(m_down_button);
	order_buttons->addStretch();
	layout->addLayout(order_buttons);

	m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(m_button_box);
}

void dict_selection_dialog_t::connect_signals()
{
	connect(m_tree, &QTreeWidget::itemChanged, this, &dict_selection_dialog_t::on_tree_item_changed);
	connect(m_up_button, &QPushButton::clicked, this, &dict_selection_dialog_t::on_move_up);
	connect(m_down_button, &QPushButton::clicked, this, &dict_selection_dialog_t::on_move_down);
	connect(m_button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

std::vector<std::string> dict_selection_dialog_t::get_selected_paths() const
{
	std::vector<std::string> result;
	for (int index = 0; index < m_order_list->count(); ++index)
		result.push_back(m_order_list->item(index)->data(Qt::UserRole).toString().toStdString());

	return result;
}

void dict_selection_dialog_t::on_tree_item_changed(QTreeWidgetItem * item, int)
{
	const auto & path_data = item->data(0, Qt::UserRole).toString();
	if (path_data.isEmpty())
		return;

	if (item->checkState(0) == Qt::Checked)
	{
		for (int index = 0; index < m_order_list->count(); ++index)
		{
			if (m_order_list->item(index)->data(Qt::UserRole).toString() == path_data)
				return;
		}

		auto * order_item = new QListWidgetItem(item->text(0));
		order_item->setData(Qt::UserRole, path_data);
		m_order_list->addItem(order_item);
	}
	else
	{
		for (int index = 0; index < m_order_list->count(); ++index)
		{
			if (m_order_list->item(index)->data(Qt::UserRole).toString() == path_data)
			{
				delete m_order_list->takeItem(index);
				break;
			}
		}
	}

	update_ok_button();
}

void dict_selection_dialog_t::on_move_up()
{
	int current_row = m_order_list->currentRow();
	if (current_row <= 0)
		return;

	auto * item = m_order_list->takeItem(current_row);
	m_order_list->insertItem(current_row - 1, item);
	m_order_list->setCurrentRow(current_row - 1);
}

void dict_selection_dialog_t::on_move_down()
{
	int current_row = m_order_list->currentRow();
	if (current_row < 0 || current_row >= m_order_list->count() - 1)
		return;

	auto * item = m_order_list->takeItem(current_row);
	m_order_list->insertItem(current_row + 1, item);
	m_order_list->setCurrentRow(current_row + 1);
}

void dict_selection_dialog_t::update_ok_button()
{
	m_button_box->button(QDialogButtonBox::Ok)->setEnabled(m_order_list->count() > 0);
}
