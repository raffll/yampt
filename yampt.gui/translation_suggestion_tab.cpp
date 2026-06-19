#include "translation_suggestion_tab.hpp"
#include "ctranslate2_provider.hpp"
#include "deepl_provider.hpp"
#include "google_provider.hpp"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

static const std::vector<std::pair<std::string, std::string>> deepl_languages = {
    {"BG", "Bulgarian"}, {"CS", "Czech"}, {"DA", "Danish"}, {"DE", "German"},
    {"EL", "Greek"}, {"ES", "Spanish"}, {"ET", "Estonian"}, {"FI", "Finnish"},
    {"FR", "French"}, {"HU", "Hungarian"}, {"ID", "Indonesian"}, {"IT", "Italian"},
    {"JA", "Japanese"}, {"KO", "Korean"}, {"LT", "Lithuanian"}, {"LV", "Latvian"},
    {"NB", "Norwegian"}, {"NL", "Dutch"}, {"PL", "Polish"}, {"PT", "Portuguese"},
    {"RO", "Romanian"}, {"RU", "Russian"}, {"SK", "Slovak"}, {"SL", "Slovenian"},
    {"SV", "Swedish"}, {"TR", "Turkish"}, {"UK", "Ukrainian"}, {"ZH", "Chinese"},
};

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

    language_combo_ = new QComboBox(this);
    top_row->addWidget(language_combo_);

    translate_btn_ = new QPushButton("Translate", this);
    translate_btn_->setFixedWidth(80);
    top_row->addWidget(translate_btn_);

    layout->addLayout(top_row);

    auto * key_row = new QHBoxLayout;
    key_row->setSpacing(4);
    key_label_ = new QLabel("API Key:", this);
    key_row->addWidget(key_label_);
    api_key_field_ = new QLineEdit(this);
    api_key_field_->setEchoMode(QLineEdit::Password);
    api_key_field_->setPlaceholderText("DeepL API key");
    key_row->addWidget(api_key_field_);
    layout->addLayout(key_row);

    result_text_ = new QPlainTextEdit(this);
    result_text_->setReadOnly(true);
    result_text_->setPlaceholderText("Translation suggestion will appear here");
    layout->addWidget(result_text_);

    counter_label_ = new QLabel(this);
    counter_label_->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
    layout->addWidget(counter_label_);

    ct2_provider_ = new ctranslate2_provider_t(this);
    deepl_provider_ = new deepl_provider_t(this);
    google_provider_ = new google_provider_t(this);

    providers_.push_back(ct2_provider_);
    providers_.push_back(deepl_provider_);
    providers_.push_back(google_provider_);

    source_combo_->addItem(QString::fromStdString(ct2_provider_->name()));
    source_combo_->addItem(QString::fromStdString(deepl_provider_->name()));
    source_combo_->addItem(QString::fromStdString(google_provider_->name()));

    connect(translate_btn_, &QPushButton::clicked, this, &translation_suggestion_tab_t::on_translate_clicked);
    connect(ct2_provider_, &ctranslate2_provider_t::translation_finished, this, &translation_suggestion_tab_t::on_result);
    connect(deepl_provider_, &deepl_provider_t::translation_finished, this, &translation_suggestion_tab_t::on_result);
    connect(google_provider_, &google_provider_t::translation_finished, this, &translation_suggestion_tab_t::on_result);
    connect(source_combo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        bool show_key = (idx == 1);
        key_label_->setVisible(show_key);
        api_key_field_->setVisible(show_key);
        rebuild_language_combo();
        update_counter_label();
    });
    connect(language_combo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &translation_suggestion_tab_t::on_language_changed);
    connect(api_key_field_, &QLineEdit::editingFinished, this, [this]() {
        deepl_provider_->set_api_key(api_key_field_->text().toStdString());
        update_counter_label();
    });

    key_label_->setVisible(false);
    api_key_field_->setVisible(false);
    rebuild_language_combo();
    update_counter_label();
}

void translation_suggestion_tab_t::set_source_text(const std::string & text)
{
    source_text_ = text;
}

void translation_suggestion_tab_t::set_models_dir(const std::string & dir)
{
    models_dir_ = dir;
    rebuild_language_combo();
}

void translation_suggestion_tab_t::set_deepl_api_key(const std::string & key)
{
    deepl_provider_->set_api_key(key);
    api_key_field_->setText(QString::fromStdString(key));
    update_counter_label();
}

std::string translation_suggestion_tab_t::deepl_api_key() const
{
    return api_key_field_->text().toStdString();
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

int translation_suggestion_tab_t::source_index() const
{
    return source_combo_->currentIndex();
}

void translation_suggestion_tab_t::set_source_index(int index)
{
    if (index >= 0 && index < source_combo_->count())
        source_combo_->setCurrentIndex(index);
}

int translation_suggestion_tab_t::language_index() const
{
    return language_combo_->currentIndex();
}

void translation_suggestion_tab_t::set_language_index(int index)
{
    if (index >= 0 && index < language_combo_->count())
        language_combo_->setCurrentIndex(index);
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

    if (idx == 0 && !ct2_provider_->is_available())
        load_model_for_language(language_combo_->currentIndex());

    if (!provider->is_available())
    {
        result_text_->setPlainText(QString::fromStdString(provider->name() + " is not available"));
        return;
    }

    int lang_idx = language_combo_->currentIndex();
    std::string target_lang = (lang_idx >= 0 && lang_idx < static_cast<int>(languages_.size()))
        ? languages_[lang_idx].code : "";

    result_text_->setPlainText("Translating...");
    translate_btn_->setEnabled(false);

    provider->translate(source_text_, target_lang);

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

void translation_suggestion_tab_t::on_language_changed(int index)
{
    if (source_combo_->currentIndex() == 0)
        load_model_for_language(index);
}

void translation_suggestion_tab_t::rebuild_language_combo()
{
    language_combo_->blockSignals(true);
    language_combo_->clear();
    languages_.clear();

    int idx = source_combo_->currentIndex();

    if (idx == 0)
    {
        QDir models_qdir(QString::fromStdString(models_dir_));
        if (models_qdir.exists())
        {
            auto entries = models_qdir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const auto & entry : entries)
            {
                auto name = entry.toStdString();
                if (name.size() < 4 || name.substr(0, 3) != "en-")
                    continue;

                auto lang_code = name.substr(3);
                auto display = QString("EN \u2192 %1").arg(QString::fromStdString(lang_code).toUpper());
                auto model_path = models_dir_ + "/" + name;

                languages_.push_back({lang_code, display.toStdString(), model_path});
                language_combo_->addItem(display);
            }
        }
    }
    else if (idx == 1 || idx == 2)
    {
        for (const auto & [code, name] : deepl_languages)
        {
            auto display = QString("EN \u2192 %1").arg(QString::fromStdString(name));
            languages_.push_back({code, display.toStdString(), ""});
            language_combo_->addItem(display);
        }
    }

    language_combo_->blockSignals(false);
}

void translation_suggestion_tab_t::load_model_for_language(int index)
{
    if (index < 0 || index >= static_cast<int>(languages_.size()))
        return;

    const auto & lang = languages_[index];
    if (lang.model_path.empty())
        return;

    ct2_provider_->load_model(lang.model_path);

    if (ct2_provider_->is_available())
        counter_label_->setText(QString::fromStdString("Model loaded: " + lang.display));
    else
        counter_label_->setText("Failed to load model");
}

void translation_suggestion_tab_t::update_counter_label()
{
    int idx = source_combo_->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(providers_.size()))
    {
        counter_label_->clear();
        return;
    }

    if (idx == 0)
    {
        if (ct2_provider_->is_available())
            counter_label_->setText("Model loaded");
        else
            counter_label_->setText("No model loaded");
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
