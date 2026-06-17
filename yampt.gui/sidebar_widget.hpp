#pragma once

#include <QWidget>
#include <string>
#include <vector>

#include "../yampt/file_list.hpp"

class QTreeWidget;
class QTreeWidgetItem;

enum class plugin_op_t
{
    make_dict,
    make_dict_with_base,
    make_base,
    convert,
    create_plugin
};

class sidebar_widget_t : public QWidget
{
    Q_OBJECT

public:
    explicit sidebar_widget_t(QWidget * parent = nullptr);

    void set_model(const sidebar_render_model_t & model);
    void update_item_text(const std::string & path, const std::string & display_text);

signals:
    void item_clicked(const std::string & path);
    void operation_requested(const std::string & path, plugin_op_t op);
    void save_requested(const std::string & path);
    void save_as_requested(const std::string & path);
    void unload_requested(const std::string & path);
    void delete_requested(const std::string & path);
    void delete_folder_requested(const std::string & folder_path);
    void remove_folder_requested(const std::string & root_path);

private:
    void on_item_clicked(QTreeWidgetItem * item, int column);
    void on_context_menu(const QPoint & pos);

    QTreeWidget * tree_ = nullptr;
};
