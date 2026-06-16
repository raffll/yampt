#include "sidebar_widget.hpp"

#include <QFileInfo>
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>

static constexpr int role_path = Qt::UserRole;
static constexpr int role_is_workspace = Qt::UserRole + 1;

sidebar_widget_t::sidebar_widget_t(QWidget * parent)
    : QWidget(parent)
{
    auto * layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_ = new QListWidget(this);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(list_);

    connect(list_, &QListWidget::itemClicked, this, &sidebar_widget_t::on_item_clicked);
    connect(list_, &QListWidget::customContextMenuRequested, this, &sidebar_widget_t::on_context_menu);
}

void sidebar_widget_t::add_section_header(const std::string & label)
{
	auto * header = new QListWidgetItem(QString::fromStdString("--- " + label + " ---"));
	header->setFlags(Qt::NoItemFlags);
	header->setForeground(QColor(100, 100, 100));
	list_->addItem(header);
}

void sidebar_widget_t::add_leaf_items(const std::vector<sidebar_render_item_t> & items, int indent)
{
	for (const auto & item : items)
	{
		QString display = QString::fromStdString(item.display_text);
		if (indent > 0)
			display = QString(indent * 2, ' ') + display;

		auto * list_item = new QListWidgetItem(display);
		list_item->setToolTip(QString::fromStdString(item.path));
		list_item->setData(role_path, QString::fromStdString(item.path));
		list_item->setData(role_is_workspace, item.is_workspace);
		list_->addItem(list_item);
	}
}

void sidebar_widget_t::set_model(const sidebar_render_model_t & model)
{
	list_->clear();

	add_section_header(model.loaded_root.label);
	add_leaf_items(model.loaded_root.items, 0);

	add_section_header(model.workspace_root.label);
	add_leaf_items(model.workspace_root.items, 0);

	for (const auto & child : model.workspace_root.children)
	{
		auto * subfolder_header = new QListWidgetItem(QString::fromStdString("  " + child.label));
		subfolder_header->setFlags(Qt::NoItemFlags);
		subfolder_header->setForeground(QColor(130, 130, 130));
		list_->addItem(subfolder_header);

		add_leaf_items(child.items, 1);
	}

	if (model.active_path.empty())
		return;

	for (int i = 0; i < list_->count(); ++i)
	{
		auto * item = list_->item(i);
		auto path = item->data(role_path).toString().toStdString();
		if (path == model.active_path)
		{
			list_->setCurrentItem(item);
			return;
		}
	}
}

void sidebar_widget_t::on_item_clicked(QListWidgetItem * item)
{
    if (!item)
        return;

    if (!(item->flags() & Qt::ItemIsSelectable))
        return;

    auto path_data = item->data(role_path);
    if (path_data.typeId() != QMetaType::QString)
        return;

    auto path = path_data.toString().toStdString();
    if (path.empty())
        return;

    emit item_clicked(path);
}

void sidebar_widget_t::on_context_menu(const QPoint & pos)
{
    auto * item = list_->itemAt(pos);
    if (!item)
        return;

    if (!(item->flags() & Qt::ItemIsSelectable))
        return;

    auto path_data = item->data(role_path);
    if (path_data.typeId() != QMetaType::QString)
        return;

    auto path = path_data.toString();
    if (path.isEmpty())
        return;

    const auto ext = QFileInfo(path).suffix().toLower();
    const auto is_workspace = item->data(role_is_workspace).toBool();
    auto path_str = path.toStdString();

    QMenu menu(this);

    if (ext == "esm" || ext == "esp")
    {
        auto * make_dict_action = menu.addAction("Make Dict");
        auto * make_dict_base_action = menu.addAction("Make Dict with Base");
        auto * make_base_action = menu.addAction("Make Base");
        menu.addSeparator();
        auto * convert_action = menu.addAction("Convert");
        auto * create_action = menu.addAction("Create");
        menu.addSeparator();

        QAction * unload_action = nullptr;
        QAction * delete_action = nullptr;

        if (is_workspace)
            delete_action = menu.addAction("Delete");
        else
            unload_action = menu.addAction("Unload");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
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
        else if (unload_action && selected == unload_action)
            emit unload_requested(path_str);
        else if (delete_action && selected == delete_action)
            emit delete_requested(path_str);
        return;
    }

    if (ext == "json" || ext == "xml")
    {
        QAction * save_action = menu.addAction("Save");
        menu.addSeparator();

        QAction * unload_action = nullptr;
        QAction * delete_action = nullptr;

        if (is_workspace)
            delete_action = menu.addAction("Delete");
        else
            unload_action = menu.addAction("Unload");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == save_action)
            emit save_requested(path_str);
        else if (unload_action && selected == unload_action)
            emit unload_requested(path_str);
        else if (delete_action && selected == delete_action)
            emit delete_requested(path_str);
    }
}
