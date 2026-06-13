#pragma once

#include <QWidget>
#include <string>
#include <vector>

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

struct workspace_section_t
{
    std::string folder_name;
    std::vector<std::string> file_names;
    std::vector<std::string> file_paths;
};

struct loaded_item_t
{
    std::string display_name;
    std::string path;
    bool is_plugin = false;
    bool is_base = false;
    bool is_dirty = false;
    int slot_index = -1;
    int plugin_index = -1;
};

class sidebar_widget_t : public QWidget
{
    Q_OBJECT

public:
    explicit sidebar_widget_t(QWidget * parent = nullptr);

    void set_loaded_items(const std::vector<loaded_item_t> & items);
    void set_workspace_sections(const std::vector<workspace_section_t> & sections);
    void set_active_slot(int slot_index);
    int get_clicked_slot_index() const;

signals:
    void slot_clicked(int slot_index);
    void save_requested(int slot_index);
    void save_as_requested(int slot_index);
    void unload_requested(int slot_index);
    void plugin_operation_requested(int plugin_index, plugin_op_t op);
    void plugin_unload_requested(int plugin_index);
    void plugin_selected();
    void workspace_file_clicked(const std::string & path);
    void workspace_delete_requested(const std::string & path);

private:
    void on_item_clicked(QListWidgetItem * item);
    void on_context_menu(const QPoint & pos);
    void rebuild_workspace_section();

    QListWidget * list_ = nullptr;
    int loaded_count_ = 0;
    int clicked_slot_index_ = -1;

    std::vector<workspace_section_t> workspace_sections_;
};
