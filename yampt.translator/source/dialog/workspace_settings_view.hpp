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

    QListWidget * m_path_list = nullptr;
    QPushButton * m_add_button = nullptr;
    QPushButton * m_remove_button = nullptr;
    QPushButton * m_move_up_button = nullptr;
    QPushButton * m_move_down_button = nullptr;
};
