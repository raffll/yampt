#include "sidebar_widget.hpp"

#include <functional>
#include <set>

#include <QFileInfo>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

static constexpr int role_path = Qt::UserRole;
static constexpr int role_root_path = Qt::UserRole + 1;
static constexpr int role_folder_path = Qt::UserRole + 2;

sidebar_widget_t::sidebar_widget_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	tree_ = new QTreeWidget(this);
	tree_->setHeaderHidden(true);
	tree_->setColumnCount(1);
	tree_->setRootIsDecorated(true);
	tree_->setContextMenuPolicy(Qt::CustomContextMenu);
	tree_->setIndentation(16);
	layout->addWidget(tree_);

	connect(tree_, &QTreeWidget::itemClicked, this, &sidebar_widget_t::on_item_clicked);
	connect(tree_, &QTreeWidget::customContextMenuRequested, this, &sidebar_widget_t::on_context_menu);
}

void sidebar_widget_t::set_model(const sidebar_render_model_t & model)
{
	std::set<std::string> expanded_roots;
	std::set<std::string> collapsed_roots;
	for (int i = 0; i < tree_->topLevelItemCount(); ++i)
	{
		auto * item = tree_->topLevelItem(i);
		const auto label = item->text(0).toStdString();
		if (item->isExpanded())
			expanded_roots.insert(label);
		else
			collapsed_roots.insert(label);
	}

	tree_->clear();

	std::function<void(QTreeWidgetItem *, const sidebar_render_node_t &)> add_node;
	add_node = [&](QTreeWidgetItem * parent, const sidebar_render_node_t & node)
	{
		for (const auto & file_item : node.items)
		{
			auto * child = new QTreeWidgetItem(parent);
			child->setText(0, QString::fromStdString(file_item.display_text));
			child->setToolTip(0, QString::fromStdString(file_item.path));
			child->setData(0, role_path, QString::fromStdString(file_item.path));
			child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

			switch (file_item.type)
			{
			case file_type_t::plugin:
				child->setForeground(0, QColor(100, 180, 100));
				break;
			case file_type_t::base_dict:
				child->setForeground(0, QColor(180, 140, 80));
				break;
			case file_type_t::user_dict:
				child->setForeground(0, QColor(100, 160, 220));
				break;
			case file_type_t::yaml_l10n:
				child->setForeground(0, QColor(180, 120, 180));
				break;
			}
		}

		for (const auto & subfolder : node.children)
		{
			auto * folder_item = new QTreeWidgetItem(parent);
			folder_item->setText(0, QString::fromStdString(subfolder.label));
			folder_item->setFlags(Qt::ItemIsEnabled);
			folder_item->setForeground(0, QColor(130, 130, 130));

			if (!subfolder.folder_path.empty())
				folder_item->setData(0, role_folder_path, QString::fromStdString(subfolder.folder_path));

			add_node(folder_item, subfolder);
			folder_item->setExpanded(true);
		}
	};

	for (const auto & root : model.roots)
	{
		auto * root_item = new QTreeWidgetItem(tree_);
		root_item->setText(0, QString::fromStdString(root.label));
		root_item->setFlags(Qt::ItemIsEnabled);

		QFont bold_font = root_item->font(0);
		bold_font.setBold(true);
		root_item->setFont(0, bold_font);

		if (root.label != workspace_label)
			root_item->setData(0, role_root_path, QString::fromStdString(root.root_path));

		add_node(root_item, root);

		if (collapsed_roots.count(root.label))
			root_item->setExpanded(false);
		else
			root_item->setExpanded(true);
	}

	if (model.active_path.empty())
		return;

	QTreeWidgetItemIterator it(tree_);
	while (*it)
	{
		const auto path = (*it)->data(0, role_path).toString().toStdString();
		if (path == model.active_path)
		{
			tree_->setCurrentItem(*it);
			return;
		}

		++it;
	}
}

void sidebar_widget_t::on_item_clicked(QTreeWidgetItem * item, int column)
{
	(void)column;

	if (!item)
		return;

	if (!(item->flags() & Qt::ItemIsSelectable))
		return;

	const auto path = item->data(0, role_path).toString().toStdString();
	if (path.empty())
		return;

	emit item_clicked(path);
}

void sidebar_widget_t::on_context_menu(const QPoint & pos)
{
	auto * item = tree_->itemAt(pos);
	if (!item)
		return;

	if (!(item->flags() & Qt::ItemIsSelectable))
	{
		const auto root_path = item->data(0, role_root_path).toString();
		if (!root_path.isEmpty())
		{
			QMenu menu(this);
			auto * remove_action = menu.addAction("Remove Folder");
			auto * selected = menu.exec(tree_->viewport()->mapToGlobal(pos));
			if (selected == remove_action)
				emit remove_folder_requested(root_path.toStdString());

			return;
		}

		const auto folder_path = item->data(0, role_folder_path).toString();
		if (!folder_path.isEmpty())
		{
			QMenu menu(this);
			auto * delete_action = menu.addAction("Delete Folder");
			auto * selected = menu.exec(tree_->viewport()->mapToGlobal(pos));
			if (selected == delete_action)
				emit delete_folder_requested(folder_path.toStdString());

			return;
		}

		return;
	}

	const auto path = item->data(0, role_path).toString();
	if (path.isEmpty())
		return;

	const auto ext = QFileInfo(path).suffix().toLower();
	auto path_str = path.toStdString();

	QMenu menu(this);

	if (ext == "esm" || ext == "esp" || ext == "omwgame" || ext == "omwaddon")
	{
		auto * make_dict_action = menu.addAction("Make Dict");
		auto * make_dict_base_action = menu.addAction("Make Dict with Base");
		auto * make_base_action = menu.addAction("Make Base");
		menu.addSeparator();
		auto * convert_action = menu.addAction("Convert");
		auto * create_action = menu.addAction("Create");
		menu.addSeparator();
		auto * delete_action = menu.addAction("Delete");

		auto * selected = menu.exec(tree_->viewport()->mapToGlobal(pos));
		if (selected == make_dict_action)
			emit operation_requested(path_str, plugin_op_t::make_dict);
		else if (selected == make_dict_base_action)
			emit operation_requested(path_str, plugin_op_t::make_dict_with_base);
		else if (selected == make_base_action)
			emit operation_requested(path_str, plugin_op_t::make_base);
		else if (selected == convert_action)
			emit operation_requested(path_str, plugin_op_t::convert);
		else if (selected == create_action)
			emit operation_requested(path_str, plugin_op_t::create_plugin);
		else if (selected == delete_action)
			emit delete_requested(path_str);

		return;
	}

	if (ext == "json" || ext == "xml")
	{
		auto * save_action = menu.addAction("Save");
		menu.addSeparator();
		auto * delete_action = menu.addAction("Delete");

		auto * selected = menu.exec(tree_->viewport()->mapToGlobal(pos));
		if (selected == save_action)
			emit save_requested(path_str);
		else if (selected == delete_action)
			emit delete_requested(path_str);

		return;
	}

	if (ext == "yaml")
	{
		auto * save_action = menu.addAction("Save");
		auto * export_action = menu.addAction("Export...");
		menu.addSeparator();
		auto * delete_action = menu.addAction("Delete");

		auto * selected = menu.exec(tree_->viewport()->mapToGlobal(pos));
		if (selected == save_action)
			emit save_requested(path_str);
		else if (selected == export_action)
			emit save_as_requested(path_str);
		else if (selected == delete_action)
			emit delete_requested(path_str);
	}
}

void sidebar_widget_t::update_item_text(const std::string & path, const std::string & display_text)
{
	QTreeWidgetItemIterator it(tree_);
	while (*it)
	{
		if ((*it)->data(0, role_path).toString().toStdString() == path)
		{
			(*it)->setText(0, QString::fromStdString(display_text));
			return;
		}

		++it;
	}
}
