#include "sidebar_view.hpp"
#include <set>
#include <QFileInfo>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

static constexpr int role_path = Qt::UserRole;
static constexpr int role_root_path = Qt::UserRole + 1;
static constexpr int role_folder_path = Qt::UserRole + 2;
static constexpr int role_is_native_yaml = Qt::UserRole + 3;

sidebar_view_t::sidebar_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_tree = new QTreeWidget(this);
	m_tree->setHeaderHidden(true);
	m_tree->setColumnCount(1);
	m_tree->setRootIsDecorated(true);
	m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tree->setIndentation(16);
	layout->addWidget(m_tree);

	connect(m_tree, &QTreeWidget::itemClicked, this, &sidebar_view_t::on_item_clicked);
	connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &sidebar_view_t::on_context_menu);
}

static QColor get_file_type_color(file_type_t type)
{
	switch (type)
	{
	case file_type_t::plugin:
		return QColor(100, 180, 100);
	case file_type_t::base_dict:
		return QColor(180, 140, 80);
	case file_type_t::user_dict:
		return QColor(100, 160, 220);
	case file_type_t::yaml_l10n:
		return QColor(180, 120, 180);
	}

	return QColor(80, 80, 80);
}

static void populate_node(QTreeWidgetItem * parent, const sidebar_render_node_t & node)
{
	for (const auto & file_item : node.items)
	{
		auto * child = new QTreeWidgetItem(parent);
		child->setText(0, QString::fromStdString(file_item.display_text));
		child->setToolTip(0, QString::fromStdString(file_item.path));
		child->setData(0, role_path, QString::fromStdString(file_item.path));
		child->setData(0, role_is_native_yaml, file_item.is_native_yaml);
		child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		child->setForeground(0, get_file_type_color(file_item.type));
	}

	for (const auto & subfolder : node.children)
	{
		auto * folder_item = new QTreeWidgetItem(parent);
		folder_item->setText(0, QString::fromStdString(subfolder.label));
		folder_item->setFlags(Qt::ItemIsEnabled);
		folder_item->setForeground(0, QColor(130, 130, 130));

		if (!subfolder.folder_path.empty())
			folder_item->setData(0, role_folder_path, QString::fromStdString(subfolder.folder_path));

		populate_node(folder_item, subfolder);
		folder_item->setExpanded(true);
	}
}

static void select_active_item(QTreeWidget * tree, const std::string & active_path)
{
	if (active_path.empty())
		return;

	QTreeWidgetItemIterator it(tree);
	while (*it)
	{
		const auto path = (*it)->data(0, role_path).toString().toStdString();
		if (path == active_path)
		{
			tree->setCurrentItem(*it);
			return;
		}

		++it;
	}
}

void sidebar_view_t::set_model(const sidebar_render_model_t & model)
{
	std::set<std::string> collapsed_roots;
	for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
	{
		auto * item = m_tree->topLevelItem(i);
		if (!item->isExpanded())
			collapsed_roots.insert(item->text(0).toStdString());
	}

	m_tree->clear();

	for (const auto & root : model.roots)
	{
		auto * root_item = new QTreeWidgetItem(m_tree);
		root_item->setText(0, QString::fromStdString(root.label));
		root_item->setFlags(Qt::ItemIsEnabled);

		QFont bold_font = root_item->font(0);
		bold_font.setBold(true);
		root_item->setFont(0, bold_font);

		if (root.label != workspace_label)
			root_item->setData(0, role_root_path, QString::fromStdString(root.root_path));

		populate_node(root_item, root);

		root_item->setExpanded(!collapsed_roots.count(root.label));
	}

	select_active_item(m_tree, model.active_path);
}

void sidebar_view_t::on_item_clicked(QTreeWidgetItem * item, int column)
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

void sidebar_view_t::on_context_menu(const QPoint & pos)
{
	auto * item = m_tree->itemAt(pos);
	if (!item)
		return;

	if (!(item->flags() & Qt::ItemIsSelectable))
	{
		show_folder_context_menu(item, pos);
		return;
	}

	const auto path = item->data(0, role_path).toString();
	if (path.isEmpty())
		return;

	const auto ext = QFileInfo(path).suffix().toLower();
	auto path_str = path.toStdString();

	if (ext == "esm" || ext == "esp" || ext == "omwgame" || ext == "omwaddon")
		show_plugin_context_menu(path_str, pos);
	else if (ext == "json" || ext == "xml")
		show_dict_context_menu(path_str, pos);
	else if (ext == "yaml")
	{
		const bool is_native = item->data(0, role_is_native_yaml).toBool();
		show_yaml_context_menu(path_str, is_native, pos);
	}
}

void sidebar_view_t::show_folder_context_menu(QTreeWidgetItem * item, const QPoint & pos)
{
	const auto root_path = item->data(0, role_root_path).toString();
	if (!root_path.isEmpty())
	{
		QMenu menu(this);
		auto * remove_action = menu.addAction(tr("Remove Folder"));
		auto * selected = menu.exec(m_tree->viewport()->mapToGlobal(pos));
		if (selected == remove_action)
			emit remove_folder_requested(root_path.toStdString());

		return;
	}

	const auto folder_path = item->data(0, role_folder_path).toString();
	if (!folder_path.isEmpty())
	{
		QMenu menu(this);
		auto * delete_action = menu.addAction(tr("Delete Folder"));
		auto * selected = menu.exec(m_tree->viewport()->mapToGlobal(pos));
		if (selected == delete_action)
			emit delete_folder_requested(folder_path.toStdString());
	}
}

void sidebar_view_t::show_plugin_context_menu(const std::string & path, const QPoint & pos)
{
	QMenu menu(this);
	auto * make_dict_action = menu.addAction(tr("Make Dictionary"));
	auto * make_base_action = menu.addAction(tr("Make Base"));
	menu.addSeparator();
	auto * convert_action = menu.addAction(tr("Convert"));
	auto * create_action = menu.addAction(tr("Create"));
	menu.addSeparator();
	auto * delete_action = menu.addAction(tr("Delete"));

	auto * selected = menu.exec(m_tree->viewport()->mapToGlobal(pos));
	if (selected == make_dict_action)
		emit operation_requested(path, plugin_op_t::make_dict);
	else if (selected == make_base_action)
		emit operation_requested(path, plugin_op_t::make_base);
	else if (selected == convert_action)
		emit operation_requested(path, plugin_op_t::convert);
	else if (selected == create_action)
		emit operation_requested(path, plugin_op_t::create_plugin);
	else if (selected == delete_action)
		emit delete_requested(path);
}

void sidebar_view_t::show_dict_context_menu(const std::string & path, const QPoint & pos)
{
	QMenu menu(this);
	auto * save_action = menu.addAction(tr("Save"));
	menu.addSeparator();
	auto * delete_action = menu.addAction(tr("Delete"));

	auto * selected = menu.exec(m_tree->viewport()->mapToGlobal(pos));
	if (selected == save_action)
		emit save_requested(path);
	else if (selected == delete_action)
		emit delete_requested(path);
}

void sidebar_view_t::show_yaml_context_menu(const std::string & path, bool is_native, const QPoint & pos)
{
	QMenu menu(this);

	QAction * save_action = nullptr;
	QAction * export_native_action = nullptr;

	if (is_native)
	{
		save_action = menu.addAction(tr("Save"));
	}
	else
	{
		export_native_action = menu.addAction(tr("Make Translation"));
		export_native_action->setToolTip(tr("Create native language YAML from this source file"));
	}

	menu.addSeparator();
	auto * delete_action = menu.addAction(tr("Delete"));

	auto * selected = menu.exec(m_tree->viewport()->mapToGlobal(pos));
	if (selected == save_action)
		emit save_requested(path);
	else if (selected == export_native_action)
		emit export_native_requested(path);
	else if (selected == delete_action)
		emit delete_requested(path);
}

void sidebar_view_t::update_item_text(const std::string & path, const std::string & display_text)
{
	QTreeWidgetItemIterator it(m_tree);
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
