#pragma once

#include <string>
#include <QWidget>

class QComboBox;
class QLineEdit;
class app_settings_t;

class language_settings_view_t : public QWidget
{
    Q_OBJECT

public:
    explicit language_settings_view_t(const std::string & dictionaries_dir, QWidget * parent = nullptr);

    void load(const app_settings_t & settings);
    void apply(app_settings_t & settings) const;

private:
    void on_native_language_changed(int index);
    void on_foreign_language_changed(int index);
    void scan_dictionaries(const std::string & directory);

    QComboBox * native_language_combo_ = nullptr;
    QComboBox * foreign_language_combo_ = nullptr;
    QComboBox * encoding_combo_ = nullptr;
    QComboBox * spell_check_combo_ = nullptr;
    QComboBox * translation_target_combo_ = nullptr;
    QComboBox * partial_dict_combo_ = nullptr;
    QLineEdit * native_tag_edit_ = nullptr;
    QLineEdit * foreign_tag_edit_ = nullptr;
    std::string dictionaries_dir_;
};
