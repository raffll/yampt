#include "dialog/translation_settings_view.hpp"
#include <io/app_settings.hpp>

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>

translation_settings_view_t::translation_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
    auto * layout = new QVBoxLayout(this);
    auto * form = new QFormLayout;

    provider_combo_ = new QComboBox(this);
    provider_combo_->addItem("CTranslate2 (local)");
    provider_combo_->addItem("DeepL");
    provider_combo_->addItem("Google Translate");
    form->addRow("Provider:", provider_combo_);

    auto * deepl_row = new QHBoxLayout;
    deepl_key_edit_ = new QLineEdit(this);
    deepl_key_edit_->setEchoMode(QLineEdit::Password);
    deepl_reveal_button_ = new QToolButton(this);
    deepl_reveal_button_->setText("Show");
    deepl_reveal_button_->setToolTip("Toggle DeepL API key visibility");
    deepl_reveal_button_->setCheckable(true);
    deepl_row->addWidget(deepl_key_edit_);
    deepl_row->addWidget(deepl_reveal_button_);
    form->addRow("DeepL API Key:", deepl_row);

    auto * google_row = new QHBoxLayout;
    google_key_edit_ = new QLineEdit(this);
    google_key_edit_->setEchoMode(QLineEdit::Password);
    google_reveal_button_ = new QToolButton(this);
    google_reveal_button_->setText("Show");
    google_reveal_button_->setToolTip("Toggle Google API key visibility");
    google_reveal_button_->setCheckable(true);
    google_row->addWidget(google_key_edit_);
    google_row->addWidget(google_reveal_button_);
    form->addRow("Google API Key:", google_row);

    layout->addLayout(form);
    layout->addStretch();

    connect(provider_combo_, &QComboBox::currentIndexChanged,
            this, &translation_settings_view_t::on_provider_changed);

    connect(deepl_reveal_button_, &QToolButton::toggled, this, [this](bool checked) {
        deepl_key_edit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        deepl_reveal_button_->setText(checked ? "Hide" : "Show");
    });

    connect(google_reveal_button_, &QToolButton::toggled, this, [this](bool checked) {
        google_key_edit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        google_reveal_button_->setText(checked ? "Hide" : "Show");
    });

    on_provider_changed(0);
}

void translation_settings_view_t::load(const app_settings_t & settings)
{
    provider_combo_->setCurrentIndex(settings.translation_source_index());
    deepl_key_edit_->setText(QString::fromStdString(settings.deepl_api_key()));
    google_key_edit_->setText(QString::fromStdString(settings.google_api_key()));
}

void translation_settings_view_t::apply(app_settings_t & settings) const
{
    settings.set_translation_source_index(provider_combo_->currentIndex());
    settings.set_deepl_api_key(deepl_key_edit_->text().toStdString());
    settings.set_google_api_key(google_key_edit_->text().toStdString());
}

void translation_settings_view_t::on_provider_changed(int index)
{
    const bool deepl_selected = (index == 1);
    const bool google_selected = (index == 2);

    deepl_key_edit_->setEnabled(deepl_selected);
    deepl_reveal_button_->setEnabled(deepl_selected);
    google_key_edit_->setEnabled(google_selected);
    google_reveal_button_->setEnabled(google_selected);
}
