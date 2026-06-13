#include "sidebar_widget.hpp"

#include <QFileInfo>
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>

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

void sidebar_widget_t::set_sections(const std::vector<sidebar_section_t> & sections)
{
    list_->clear();

    for (const auto & section : sections)
    {
        auto * header = new QListWidgetItem(QString::fromStdString("--- " + section.header + " ---"));
        header->setFlags(Qt::NoItemFlags);
        header->setForeground(QColor(100, 100, 100));
        list_->addItem(header);

        for (const auto & item : section.items)
        {
            QString display = QString::fromStdString(item.display_name);

            if (item.is_dirty)
                display = "* " + display;

            if (item.is_base)
                display += " [BASE]";

            auto * list_item = new QListWidgetItem(display);
            list_item->setToolTip(QString::fromStdString(item.path));
            list_item->setData(Qt::UserRole, item.slot_index);
            list_item->setData(Qt::UserRole + 1, item.is_plugin ? 2 : 0);
            list_item->setData(Qt::UserRole + 2, QString::fromStdString(item.path));
            list_item->setData(Qt::UserRole + 3, item.plugin_index);
            list_->addItem(list_item);
        }
    }
}

void sidebar_widget_t::set_active_slot(int slot_index)
{
    list_->clearSelection();

    if (slot_index < 0)
        return;

    for (int i = 0; i < list_->count(); ++i)
    {
        auto * item = list_->item(i);
        if (item->data(Qt::UserRole).toInt() == slot_index &&
            item->data(Qt::UserRole + 1).toInt() != 2 &&
            (item->flags() & Qt::ItemIsSelectable))
        {
            list_->setCurrentItem(item);
            return;
        }
    }
}

int sidebar_widget_t::get_clicked_slot_index() const
{
    return clicked_slot_index_;
}

void sidebar_widget_t::on_item_clicked(QListWidgetItem * item)
{
    if (!item)
        return;

    if (!(item->flags() & Qt::ItemIsSelectable))
        return;

    int kind = item->data(Qt::UserRole + 1).toInt();

    if (kind == 2)
    {
        emit plugin_selected();
        return;
    }

    const auto path = item->data(Qt::UserRole + 2).toString().toStdString();
    int slot_index = item->data(Qt::UserRole).toInt();

    if (slot_index >= 0)
    {
        clicked_slot_index_ = slot_index;
        emit slot_clicked(slot_index);
        return;
    }

    auto dot = path.rfind('.');
    if (dot == std::string::npos)
        return;

    auto ext = path.substr(dot);
    if (ext == ".json" || ext == ".xml")
    {
        emit workspace_file_clicked(path);
        return;
    }

    emit plugin_selected();
}

void sidebar_widget_t::on_context_menu(const QPoint & pos)
{
    auto * item = list_->itemAt(pos);
    if (!item)
        return;

    if (!(item->flags() & Qt::ItemIsSelectable))
        return;

    int kind = item->data(Qt::UserRole + 1).toInt();
    int slot_index = item->data(Qt::UserRole).toInt();
    int plugin_index = item->data(Qt::UserRole + 3).toInt();
    const auto path = item->data(Qt::UserRole + 2).toString();
    const auto ext = QFileInfo(path).suffix().toLower();

    QMenu menu(this);

    if (kind == 2)
    {
        auto * make_dict_action = menu.addAction("Make Dict");
        auto * make_dict_base_action = menu.addAction("Make Dict with Base");
        auto * make_base_action = menu.addAction("Make Base");
        menu.addSeparator();
        auto * convert_action = menu.addAction("Convert");
        auto * create_action = menu.addAction("Create");
        menu.addSeparator();
        auto * unload_action = menu.addAction("Unload");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == make_dict_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::make_dict);
        else if (selected == make_dict_base_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::make_dict_with_base);
        else if (selected == make_base_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::make_base);
        else if (selected == convert_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::convert);
        else if (selected == create_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::create_plugin);
        else if (selected == unload_action)
            emit plugin_unload_requested(plugin_index);
        return;
    }

    if (ext == "esm" || ext == "esp")
    {
        auto * make_dict_action = menu.addAction("Make Dict");
        auto * make_dict_base_action = menu.addAction("Make Dict with Base");
        auto * make_base_action = menu.addAction("Make Base");
        menu.addSeparator();
        auto * convert_action = menu.addAction("Convert");
        auto * create_action = menu.addAction("Create");
        menu.addSeparator();
        auto * delete_action = menu.addAction("Delete");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == make_dict_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::make_dict);
        else if (selected == make_dict_base_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::make_dict_with_base);
        else if (selected == make_base_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::make_base);
        else if (selected == convert_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::convert);
        else if (selected == create_action)
            emit plugin_operation_requested(plugin_index, plugin_op_t::create_plugin);
        else if (selected == delete_action)
            emit workspace_delete_requested(path.toStdString());
        return;
    }

    if (ext == "json" || ext == "xml")
    {
        auto * save_action = menu.addAction("Save");
        menu.addSeparator();

        QAction * unload_action = nullptr;
        QAction * delete_action = nullptr;

        if (slot_index >= 0)
            unload_action = menu.addAction("Unload");
        else
            delete_action = menu.addAction("Delete");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == save_action)
        {
            if (slot_index >= 0)
                emit save_requested(slot_index);
            else
                emit workspace_save_requested(path.toStdString());
        }
        else if (unload_action && selected == unload_action)
            emit unload_requested(slot_index);
        else if (delete_action && selected == delete_action)
            emit workspace_delete_requested(path.toStdString());
    }
}
