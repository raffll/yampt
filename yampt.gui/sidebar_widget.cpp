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

void sidebar_widget_t::rebuild(const std::vector<std::string> & user_names,
    const std::vector<std::string> & user_paths, const std::vector<bool> & user_dirty,
    const std::vector<std::string> & base_names, const std::vector<std::string> & base_paths)
{
    list_->clear();
    user_count_ = static_cast<int>(user_names.size());
    base_count_ = static_cast<int>(base_names.size());

    auto * user_header = new QListWidgetItem("--- User Dicts ---");
    user_header->setFlags(Qt::NoItemFlags);
    user_header->setForeground(QColor(100, 100, 100));
    list_->addItem(user_header);

    for (int i = 0; i < static_cast<int>(user_names.size()); ++i)
    {
        QString display = QString::fromStdString(user_names[i]);
        if (i < static_cast<int>(user_dirty.size()) && user_dirty[i])
            display = "* " + display;

        auto * item = new QListWidgetItem(display);
        if (i < static_cast<int>(user_paths.size()))
        {
            item->setToolTip(QString::fromStdString(user_paths[i]));
            item->setData(Qt::UserRole + 2, QString::fromStdString(user_paths[i]));
        }

        item->setData(Qt::UserRole, i);
        item->setData(Qt::UserRole + 1, 0);
        list_->addItem(item);
    }

    auto * base_header = new QListWidgetItem("--- Base Dicts ---");
    base_header->setFlags(Qt::NoItemFlags);
    base_header->setForeground(QColor(100, 100, 100));
    list_->addItem(base_header);

    for (int i = 0; i < static_cast<int>(base_names.size()); ++i)
    {
        auto * item = new QListWidgetItem(QString::fromStdString(base_names[i]));
        if (i < static_cast<int>(base_paths.size()))
        {
            item->setToolTip(QString::fromStdString(base_paths[i]));
            item->setData(Qt::UserRole + 2, QString::fromStdString(base_paths[i]));
        }

        item->setData(Qt::UserRole, user_count_ + i);
        item->setData(Qt::UserRole + 1, 1);
        list_->addItem(item);
    }

    rebuild_plugin_workspace_section();
}

void sidebar_widget_t::set_plugin_slots(const std::vector<std::string> & names, const std::vector<std::string> & paths)
{
    plugin_names_ = names;
    plugin_paths_ = paths;
    plugin_count_ = static_cast<int>(names.size());
    rebuild_plugin_workspace_section();
}

void sidebar_widget_t::set_workspace_sections(const std::vector<workspace_section_t> & sections)
{
    workspace_sections_ = sections;
    rebuild_plugin_workspace_section();
}

void sidebar_widget_t::rebuild_plugin_workspace_section()
{
    int keep_count = 1 + user_count_ + 1 + base_count_;

    while (list_->count() > keep_count)
        delete list_->takeItem(list_->count() - 1);

    auto * plugin_header = new QListWidgetItem("--- Plugins ---");
    plugin_header->setFlags(Qt::NoItemFlags);
    plugin_header->setForeground(QColor(100, 100, 100));
    list_->addItem(plugin_header);

    for (int i = 0; i < static_cast<int>(plugin_names_.size()); ++i)
    {
        auto * item = new QListWidgetItem(QString::fromStdString(plugin_names_[i]));
        if (i < static_cast<int>(plugin_paths_.size()))
        {
            item->setToolTip(QString::fromStdString(plugin_paths_[i]));
            item->setData(Qt::UserRole + 2, QString::fromStdString(plugin_paths_[i]));
        }

        item->setData(Qt::UserRole, i);
        item->setData(Qt::UserRole + 1, 2);
        list_->addItem(item);
    }

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
            auto * item = new QListWidgetItem(QString::fromStdString(section.file_names[i]));
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

void sidebar_widget_t::set_active_slot(int index)
{
    list_->clearSelection();

    if (index < 0)
        return;

    for (int i = 0; i < list_->count(); ++i)
    {
        auto * item = list_->item(i);
        if (item->data(Qt::UserRole).toInt() == index && (item->flags() & Qt::ItemIsSelectable))
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
    int index = item->data(Qt::UserRole).toInt();

    QMenu menu(this);

    if (kind == 0)
    {
        auto * save_action = menu.addAction("Save");
        auto * save_as_action = menu.addAction("Save As...");
        auto * unload_action = menu.addAction("Unload");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == save_action)
            emit save_requested(index);
        else if (selected == save_as_action)
            emit save_as_requested(index);
        else if (selected == unload_action)
            emit unload_requested(index);
    }
    else if (kind == 1)
    {
        auto * remove_action = menu.addAction("Remove");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == remove_action)
            emit remove_requested(index);
    }
    else if (kind == 2)
    {
        auto * make_dict_action = menu.addAction("Make Dict");
        auto * make_dict_base_action = menu.addAction("Make Dict with Base");
        auto * make_base_action = menu.addAction("Make Base");
        auto * convert_action = menu.addAction("Convert");
        auto * create_action = menu.addAction("Create");
        menu.addSeparator();
        auto * unload_action = menu.addAction("Unload");

        bool has_dicts = (user_count_ + base_count_) > 0;
        make_dict_base_action->setEnabled(has_dicts);
        make_base_action->setEnabled(plugin_count_ >= 2);
        convert_action->setEnabled(has_dicts);
        create_action->setEnabled(has_dicts);

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == make_dict_action)
            emit plugin_operation_requested(index, plugin_op_t::make_dict);
        else if (selected == make_dict_base_action)
            emit plugin_operation_requested(index, plugin_op_t::make_dict_with_base);
        else if (selected == make_base_action)
            emit plugin_operation_requested(index, plugin_op_t::make_base);
        else if (selected == convert_action)
            emit plugin_operation_requested(index, plugin_op_t::convert);
        else if (selected == create_action)
            emit plugin_operation_requested(index, plugin_op_t::create_plugin);
        else if (selected == unload_action)
            emit plugin_unload_requested(index);
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
            auto * convert_action = menu.addAction("Convert");
            auto * create_action = menu.addAction("Create");

            bool has_dicts = (user_count_ + base_count_) > 0;
            make_dict_base_action->setEnabled(has_dicts);
            make_base_action->setEnabled(plugin_count_ >= 2);
            convert_action->setEnabled(has_dicts);
            create_action->setEnabled(has_dicts);

            auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
            if (selected == make_dict_action)
                emit plugin_operation_requested(-(index + 1), plugin_op_t::make_dict);
            else if (selected == make_dict_base_action)
                emit plugin_operation_requested(-(index + 1), plugin_op_t::make_dict_with_base);
            else if (selected == make_base_action)
                emit plugin_operation_requested(-(index + 1), plugin_op_t::make_base);
            else if (selected == convert_action)
                emit plugin_operation_requested(-(index + 1), plugin_op_t::convert);
            else if (selected == create_action)
                emit plugin_operation_requested(-(index + 1), plugin_op_t::create_plugin);
        }
        else if (ext == "json" || ext == "xml")
        {
            auto * save_action = menu.addAction("Save");
            auto * save_as_action = menu.addAction("Save As...");
            auto * unload_action = menu.addAction("Unload");

            auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
            if (selected == save_action)
                emit save_requested(index);
            else if (selected == save_as_action)
                emit save_as_requested(index);
            else if (selected == unload_action)
                emit unload_requested(index);
        }
    }
}
