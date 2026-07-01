#pragma once

#include <string>
#include <vector>
#include <QDialog>

class QDialogButtonBox;
class QListWidget;
class QPushButton;
class QStackedWidget;
class app_settings_t;
class editor_paths_view_t;
class language_settings_view_t;
class translation_settings_view_t;
class shortcuts_settings_view_t;
class workspace_settings_view_t;

class settings_dialog_t : public QDialog
{
    Q_OBJECT

public:
    enum class context_t { translator, editor };

    explicit settings_dialog_t(app_settings_t & settings,
                                const std::string & dictionaries_dir,
                                context_t context,
                                QWidget * parent = nullptr);

signals:
    void settings_applied(const std::string & changed_category);

private:
    void apply_all();
    void update_ok_button_state();

    app_settings_t & settings_;
    context_t context_;

    QListWidget * category_list_ = nullptr;
    QStackedWidget * content_stack_ = nullptr;
    QDialogButtonBox * button_box_ = nullptr;
    QPushButton * apply_button_ = nullptr;

    language_settings_view_t * language_view_ = nullptr;
    translation_settings_view_t * translation_view_ = nullptr;
    shortcuts_settings_view_t * shortcuts_view_ = nullptr;
    workspace_settings_view_t * workspace_view_ = nullptr;
    editor_paths_view_t * editor_paths_view_ = nullptr;
};
