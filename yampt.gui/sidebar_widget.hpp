#pragma once

#include <QWidget>
#include <vector>

class QListWidget;
class QListWidgetItem;

class sidebar_widget_t : public QWidget
{
    Q_OBJECT

public:
    explicit sidebar_widget_t(QWidget * parent = nullptr);

    void rebuild(const std::vector<std::string> & user_names, const std::vector<std::string> & user_paths,
        const std::vector<bool> & user_dirty,
        const std::vector<std::string> & base_names, const std::vector<std::string> & base_paths);

    void set_active_slot(int index);
    int get_clicked_slot_index() const;

signals:
    void slot_clicked(int slot_index);
    void save_requested(int slot_index);
    void save_as_requested(int slot_index);
    void unload_requested(int slot_index);
    void remove_requested(int slot_index);

private:
    void on_item_clicked(QListWidgetItem * item);
    void on_context_menu(const QPoint & pos);

    QListWidget * list_ = nullptr;
    int user_count_ = 0;
    int clicked_slot_index_ = -1;
};
