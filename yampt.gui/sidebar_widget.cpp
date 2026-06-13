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

void sidebar_widget_t::set_loaded_items(const std::vector<loaded_item_t> & items)
{
    list_->clear();
    loaded_count_ = 0;

    auto * header = new QListWidgetItem("--- Loaded ---");
    header->setFlags(Qt::NoItemFlags);
    header->setForeground(QColor(100, 100, 100));
    list_->addItem(header);

    for (const auto & item : items)
    {
        QString display = QString::fromStdString(item.display_name);

        if (item.is_dirty)
            display = "* " + display;

        if (item.is_base)
            display += " [BASE]";

        auto * list_item = new QListWidgetItem(display);
        list_item->setToolTip(QString::fromStdString(item.path));
        list_item->setData(Qt::UserRole, item.slot_index);
        list_item->setData(Qt::UserRole + 1, item.is_plugin ? 2 : (item.is_base ? 1 : 0));
        list_item->setData(Qt::UserRole + 2, QString::fromStdString(item.path));
        list_item->setData(Qt::UserRole + 3, item.plugin_index);
        list_->addItem(list_item);
        loaded_count_++;
    }

    rebuild_workspace_section();
}

void sidebar_widget_t::set_workspace_sections(const std::vector<workspace_section_t> & sections)
{
    workspace_sections_ = sections;
    rebuild_workspace_section();
}

void sidebar_widget_t::rebuild_workspace_section()
{
    int keep_count = 1 + loaded_count_;

    while (list_->count() > keep_count)
        delete list_->takeItem(list_->count() - 1);

    int ws_flat_index = 0;
    for (const auto & section : workspace_sections_)
    {
        QString header_text;
        if (section.folder_name.empty())
            header_text = "--- Workspace ---";
        else
            header_text = "--- " + QString::fromStdString(section.folder_name) + " ---";

        auto * section_header = new QListWidgetItem(header_text);
        section_header->setFlags(Qt::NoItemFlags);
        section_header->setForeground(QColor(100, 100, 100));
        list_->addItem(section_header);

        for (int i = 0; i < static_cast<int>(section.file_names.size()); ++i)
        {
            auto display = QString::fromStdString(section.file_names[i]);
            const auto & file_name = section.file_names[i];

            if (file_name.find("_BASE_") != std::string::npos)
                display += " [BASE]";

            auto * item = new QListWidgetItem(display);
            if (i < static_cast<int>(section.file_paths.size()))
            {
                item->setToolTip(QString::fromStdString(section.file_paths[i]));
                item->setData(Qt::UserRole + 2, QString::fromStdString(section.file_paths[i]));
            }

            item->setData(Qt::UserRole, ws_flat_index);
            item->setData(Qt::UserRole + 1, 3);
            list_->addItem(item);
            ++ws_flat_index;
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
            item->data(Qt::UserRole + 1).toInt() != 3 &&
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

    if (kind == 3)
    {
        const auto path = item->data(Qt::UserRole + 2).toString().toStdString();
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
        return;
    }

    clicked_slot_index_ = item->data(Qt::UserRole).toInt();
    emit slot_clicked(clicked_slot_index_);
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

    QMenu menu(this);

    if (kind == 0 || kind == 1)
    {
        auto * save_action = menu.addAction("Save");
        auto * save_as_action = menu.addAction("Save As...");
        menu.addSeparator();
        auto * unload_action = menu.addAction("Unload");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == save_action)
            emit save_requested(slot_index);
        else if (selected == save_as_action)
            emit save_as_requested(slot_index);
        else if (selected == unload_action)
            emit unload_requested(slot_index);
    }
    else if (kind == 2)
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
    }
    else if (kind == 3)
    {
        const auto & path = item->data(Qt::UserRole + 2).toString();
        const auto & ext = QFileInfo(path).suffix().toLower();

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
                emit plugin_operation_requested(-(slot_index + 1), plugin_op_t::make_dict);
            else if (selected == make_dict_base_action)
                emit plugin_operation_requested(-(slot_index + 1), plugin_op_t::make_dict_with_base);
            else if (selected == make_base_action)
                emit plugin_operation_requested(-(slot_index + 1), plugin_op_t::make_base);
            else if (selected == convert_action)
                emit plugin_operation_requested(-(slot_index + 1), plugin_op_t::convert);
            else if (selected == create_action)
                emit plugin_operation_requested(-(slot_index + 1), plugin_op_t::create_plugin);
            else if (selected == delete_action)
                emit workspace_delete_requested(path.toStdString());
        }
        else if (ext == "json" || ext == "xml")
        {
            auto * delete_action = menu.addAction("Delete");

            auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
            if (selected == delete_action)
                emit workspace_delete_requested(path.toStdString());
        }
    }
}
