#pragma once

#include <QDialog>
#include <string>
#include <vector>

class QComboBox;

class first_run_dialog_t : public QDialog
{
    Q_OBJECT

public:
    explicit first_run_dialog_t(const std::vector<std::string> & spell_languages, QWidget * parent = nullptr);

    int selected_encoding_index() const;
    int selected_spell_lang_index() const;

private:
    QComboBox * encoding_combo_ = nullptr;
    QComboBox * spell_lang_combo_ = nullptr;
};
