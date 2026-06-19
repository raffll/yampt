#pragma once

#include "translation_provider.hpp"

#include <QWidget>
#include <memory>
#include <string>
#include <vector>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;

class ctranslate2_provider_t;
class deepl_provider_t;

class translation_suggestion_tab_t : public QWidget
{
    Q_OBJECT

public:
    explicit translation_suggestion_tab_t(QWidget * parent = nullptr);

    void set_source_text(const std::string & text);
    void set_target_language(const std::string & lang);

    void set_deepl_api_key(const std::string & key);
    void set_deepl_chars_used(int chars);
    int deepl_chars_used() const;

    bool load_ctranslate2_model(const std::string & model_path);

    int source_index() const;
    void set_source_index(int index);

private slots:
    void on_translate_clicked();
    void on_result(translation_suggestion_t result);

private:
    void update_counter_label();

    QComboBox * source_combo_ = nullptr;
    QPushButton * translate_btn_ = nullptr;
    QPlainTextEdit * result_text_ = nullptr;
    QLabel * counter_label_ = nullptr;

    std::string source_text_;
    std::string target_lang_;

    ctranslate2_provider_t * ct2_provider_ = nullptr;
    deepl_provider_t * deepl_provider_ = nullptr;
    std::vector<translation_provider_t *> providers_;
};
