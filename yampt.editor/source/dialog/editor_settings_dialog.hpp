#pragma once

#include <QDialog>

class QDialogButtonBox;
class app_settings_t;
class editor_paths_view_t;

class editor_settings_dialog_t : public QDialog
{
    Q_OBJECT

public:
    explicit editor_settings_dialog_t(app_settings_t & settings, QWidget * parent = nullptr);

private:
    void apply_all();

    app_settings_t & settings_;
    editor_paths_view_t * paths_view_ = nullptr;
    QDialogButtonBox * button_box_ = nullptr;
};
