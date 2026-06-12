#include "sidebar_widget.hpp"

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
            item->setToolTip(QString::fromStdString(user_paths[i]));

        item->setData(Qt::UserRole, i);
        item->setData(Qt::UserRole + 1, true);
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
            item->setToolTip(QString::fromStdString(base_paths[i]));

        item->setData(Qt::UserRole, user_count_ + i);
        item->setData(Qt::UserRole + 1, false);
        list_->addItem(item);
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

    int slot_index = item->data(Qt::UserRole).toInt();
    bool is_user = item->data(Qt::UserRole + 1).toBool();

    QMenu menu(this);

    if (is_user)
    {
        auto * save_action = menu.addAction("Save");
        auto * save_as_action = menu.addAction("Save As...");
        auto * unload_action = menu.addAction("Unload");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == save_action)
            emit save_requested(slot_index);
        else if (selected == save_as_action)
            emit save_as_requested(slot_index);
        else if (selected == unload_action)
            emit unload_requested(slot_index);
    }
    else
    {
        auto * remove_action = menu.addAction("Remove");

        auto * selected = menu.exec(list_->viewport()->mapToGlobal(pos));
        if (selected == remove_action)
            emit remove_requested(slot_index);
    }
}
