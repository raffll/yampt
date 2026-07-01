#pragma once

#include <QWidget>

class QListWidget;
class QPushButton;
class app_settings_t;

class workspace_settings_view_t : public QWidget
{
    Q_OBJECT

public:
    explicit workspace_settings_view_t(QWidget * parent = nullptr);

    void load(const app_settings_t & settings);
    void apply(app_settings_t & settings) const;

private:
    void on_add_clicked();
    void on_remove_clicked();
    void on_move_up_clicked();
    void on_move_down_clicked();
    void update_button_states();

    QListWidget * path_list_ = nullptr;
    QPushButton * add_button_ = nullptr;
    QPushButton * remove_button_ = nullptr;
    QPushButton * move_up_button_ = nullptr;
    QPushButton * move_down_button_ = nullptr;
};
