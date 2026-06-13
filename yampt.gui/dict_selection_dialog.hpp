#pragma once

#include "dict_workspace.hpp"

#include <QDialog>
#include <string>
#include <vector>

class QListWidget;
class QPushButton;
class QDialogButtonBox;

class dict_selection_dialog_t : public QDialog
{
    Q_OBJECT

public:
    struct dict_entry_t
    {
        std::string name;
        std::string path;
        dict_kind_t kind;
        bool checked = false;
    };

    explicit dict_selection_dialog_t(const std::vector<dict_entry_t> & entries, QWidget * parent = nullptr);

    std::vector<std::string> get_selected_paths() const;

private:
    void on_move_up();
    void on_move_down();
    void on_check_changed();
    void update_ok_button();

    QListWidget * list_ = nullptr;
    QPushButton * up_button_ = nullptr;
    QPushButton * down_button_ = nullptr;
    QDialogButtonBox * button_box_ = nullptr;
};
