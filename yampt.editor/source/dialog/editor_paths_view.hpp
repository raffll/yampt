#pragma once

#include <string>
#include <QWidget>

class QLineEdit;
class QPushButton;
class app_settings_t;

class editor_paths_view_t : public QWidget
{
    Q_OBJECT

public:
    explicit editor_paths_view_t(QWidget * parent = nullptr);

    void load(const app_settings_t & settings);
    void apply(app_settings_t & settings) const;

private:
    void on_browse_openmw();
    void on_browse_mo2();

    QLineEdit * openmw_data_edit_ = nullptr;
    QPushButton * openmw_browse_button_ = nullptr;
    QLineEdit * mo2_profile_edit_ = nullptr;
    QPushButton * mo2_browse_button_ = nullptr;
};
