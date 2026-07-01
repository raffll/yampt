#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;
class QToolButton;
class app_settings_t;

class translation_settings_view_t : public QWidget
{
    Q_OBJECT

public:
    explicit translation_settings_view_t(QWidget * parent = nullptr);

    void load(const app_settings_t & settings);
    void apply(app_settings_t & settings) const;

private:
    void on_provider_changed(int index);

    QComboBox * provider_combo_ = nullptr;
    QLineEdit * deepl_key_edit_ = nullptr;
    QToolButton * deepl_reveal_button_ = nullptr;
    QLineEdit * google_key_edit_ = nullptr;
    QToolButton * google_reveal_button_ = nullptr;
};
