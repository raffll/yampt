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

class sidebar_widget_t : public QWidget
{
    Q_OBJECT

public:
    explicit sidebar_widget_t(QWidget * parent = nullptr);

    void rebuild(const std::vector<std::string> & user_names, const std::vector<std::string> & user_paths,
        const std::vector<bool> & user_dirty,
        const std::vector<std::string> & base_names, const std::vector<std::string> & base_paths);

    void set_plugin_slots(const std::vector<std::string> & names, const std::vector<std::string> & paths);
    void set_workspace_sections(const std::vector<workspace_section_t> & sections);
    void set_active_slot(int index);
    int get_clicked_slot_index() const;

signals:
    void slot_clicked(int slot_index);
    void save_requested(int slot_index);
    void save_as_requested(int slot_index);
    void unload_requested(int slot_index);
    void remove_requested(int slot_index);
    void plugin_operation_requested(int plugin_index, plugin_op_t op);
    void plugin_unload_requested(int plugin_index);
    void workspace_file_clicked(const std::string & path);

private:
    void on_item_clicked(QListWidgetItem * item);
    void on_context_menu(const QPoint & pos);
    void rebuild_plugin_workspace_section();

    QListWidget * list_ = nullptr;
    int user_count_ = 0;
    int base_count_ = 0;
    int plugin_count_ = 0;
    int clicked_slot_index_ = -1;

    std::vector<std::string> plugin_names_;
    std::vector<std::string> plugin_paths_;
    std::vector<workspace_section_t> workspace_sections_;
};
