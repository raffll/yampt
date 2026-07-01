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

    QComboBox * m_native_language_combo = nullptr;
    QComboBox * m_foreign_language_combo = nullptr;
    QComboBox * m_encoding_combo = nullptr;
    QComboBox * m_spell_check_combo = nullptr;
    QComboBox * m_translation_target_combo = nullptr;
    QComboBox * m_partial_dict_combo = nullptr;
    QLineEdit * m_native_tag_edit = nullptr;
    QLineEdit * m_foreign_tag_edit = nullptr;
    std::string m_dictionaries_dir;
};
