#include "translation_suggestion_tab.hpp"
#include "ctranslate2_provider.hpp"
#include "deepl_provider.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

translation_suggestion_tab_t::translation_suggestion_tab_t(QWidget * parent)
    : QWidget(parent)
{
    auto * layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto * top_row = new QHBoxLayout;
    top_row->setSpacing(4);

    source_combo_ = new QComboBox(this);
    top_row->addWidget(source_combo_);

    translate_btn_ = new QPushButton("Translate", this);
    translate_btn_->setFixedWidth(80);
    top_row->addWidget(translate_btn_);

    layout->addLayout(top_row);

    result_text_ = new QPlainTextEdit(this);
    result_text_->setReadOnly(true);
    result_text_->setPlaceholderText("Translation suggestion will appear here");
    layout->addWidget(result_text_);

    counter_label_ = new QLabel(this);
    counter_label_->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
    layout->addWidget(counter_label_);

    ct2_provider_ = new ctranslate2_provider_t(this);
    deepl_provider_ = new deepl_provider_t(this);

    providers_.push_back(ct2_provider_);
    providers_.push_back(deepl_provider_);

    source_combo_->addItem(QString::fromStdString(ct2_provider_->name()));
    source_combo_->addItem(QString::fromStdString(deepl_provider_->name()));

    connect(translate_btn_, &QPushButton::clicked, this, &translation_suggestion_tab_t::on_translate_clicked);
    connect(ct2_provider_, &ctranslate2_provider_t::translation_finished, this, &translation_suggestion_tab_t::on_result);
    connect(deepl_provider_, &deepl_provider_t::translation_finished, this, &translation_suggestion_tab_t::on_result);
    connect(source_combo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        update_counter_label();
    });

    update_counter_label();
}

void translation_suggestion_tab_t::set_source_text(const std::string & text)
{
    source_text_ = text;
}

void translation_suggestion_tab_t::set_target_language(const std::string & lang)
{
    target_lang_ = lang;
}

void translation_suggestion_tab_t::set_deepl_api_key(const std::string & key)
{
    deepl_provider_->set_api_key(key);
    update_counter_label();
}

void translation_suggestion_tab_t::set_deepl_chars_used(int chars)
{
    deepl_provider_->set_chars_used(chars);
    update_counter_label();
}

int translation_suggestion_tab_t::deepl_chars_used() const
{
    return deepl_provider_->chars_used();
}

bool translation_suggestion_tab_t::load_ctranslate2_model(const std::string & model_path)
{
    return ct2_provider_->load_model(model_path);
}

int translation_suggestion_tab_t::source_index() const
{
    return source_combo_->currentIndex();
}

void translation_suggestion_tab_t::set_source_index(int index)
{
    if (index >= 0 && index < source_combo_->count())
        source_combo_->setCurrentIndex(index);
}

void translation_suggestion_tab_t::on_translate_clicked()
{
    if (source_text_.empty())
    {
        result_text_->setPlainText("No source text");
        return;
    }

    int idx = source_combo_->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(providers_.size()))
        return;

    auto * provider = providers_[idx];
    if (!provider->is_available())
    {
        result_text_->setPlainText(QString::fromStdString(provider->name() + " is not available"));
        return;
    }

    result_text_->setPlainText("Translating...");
    translate_btn_->setEnabled(false);

    provider->translate(source_text_, target_lang_);

    if (!provider->is_async())
        translate_btn_->setEnabled(true);
}

void translation_suggestion_tab_t::on_result(translation_suggestion_t result)
{
    translate_btn_->setEnabled(true);

    if (result.success)
        result_text_->setPlainText(QString::fromStdString(result.text));
    else
        result_text_->setPlainText(QString::fromStdString("Error: " + result.error));

    update_counter_label();
}

void translation_suggestion_tab_t::update_counter_label()
{
    int idx = source_combo_->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(providers_.size()))
    {
        counter_label_->clear();
        return;
    }

    auto * provider = providers_[idx];
    if (!provider->has_quota())
    {
        counter_label_->clear();
        return;
    }

    int remaining = provider->remaining_quota();
    counter_label_->setText(QString("%1 chars remaining").arg(remaining));
}
