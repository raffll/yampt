#pragma once

#include <QWidget>
#include <string>
#include <vector>

#include "../yampt/file_list.hpp"

class QListWidget;
class QListWidgetItem;

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

signals:
    void item_clicked(const std::string & path);
    void operation_requested(const std::string & path, plugin_op_t op);
    void save_requested(const std::string & path);
    void unload_requested(const std::string & path);
    void delete_requested(const std::string & path);

private:
    void on_item_clicked(QListWidgetItem * item);
    void on_context_menu(const QPoint & pos);
    void add_section_header(const std::string & label);
    void add_leaf_items(const std::vector<sidebar_render_item_t> & items, int indent);

    QListWidget * list_ = nullptr;
};
