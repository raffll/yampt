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

    m_provider_combo = new QComboBox(this);
    m_provider_combo->addItem("CTranslate2 (local)");
    m_provider_combo->addItem("DeepL");
    m_provider_combo->addItem("Google Translate");
    form->addRow("Provider:", m_provider_combo);

    auto * deepl_row = new QHBoxLayout;
    m_deepl_key_edit = new QLineEdit(this);
    m_deepl_key_edit->setEchoMode(QLineEdit::Password);
    m_deepl_reveal_button = new QToolButton(this);
    m_deepl_reveal_button->setText("Show");
    m_deepl_reveal_button->setToolTip("Toggle DeepL API key visibility");
    m_deepl_reveal_button->setCheckable(true);
    deepl_row->addWidget(m_deepl_key_edit);
    deepl_row->addWidget(m_deepl_reveal_button);
    form->addRow("DeepL API Key:", deepl_row);

    auto * google_row = new QHBoxLayout;
    m_google_key_edit = new QLineEdit(this);
    m_google_key_edit->setEchoMode(QLineEdit::Password);
    m_google_reveal_button = new QToolButton(this);
    m_google_reveal_button->setText("Show");
    m_google_reveal_button->setToolTip("Toggle Google API key visibility");
    m_google_reveal_button->setCheckable(true);
    google_row->addWidget(m_google_key_edit);
    google_row->addWidget(m_google_reveal_button);
    form->addRow("Google API Key:", google_row);

    layout->addLayout(form);
    layout->addStretch();

    connect(m_provider_combo, &QComboBox::currentIndexChanged,
            this, &translation_settings_view_t::on_provider_changed);

    connect(m_deepl_reveal_button, &QToolButton::toggled, this, [this](bool checked) {
        m_deepl_key_edit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        m_deepl_reveal_button->setText(checked ? "Hide" : "Show");
    });

    connect(m_google_reveal_button, &QToolButton::toggled, this, [this](bool checked) {
        m_google_key_edit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        m_google_reveal_button->setText(checked ? "Hide" : "Show");
    });

    on_provider_changed(0);
}

void translation_settings_view_t::load(const app_settings_t & settings)
{
    m_provider_combo->setCurrentIndex(settings.translation_source_index());
    m_deepl_key_edit->setText(QString::fromStdString(settings.deepl_api_key()));
    m_google_key_edit->setText(QString::fromStdString(settings.google_api_key()));
}

void translation_settings_view_t::apply(app_settings_t & settings) const
{
    settings.set_translation_source_index(m_provider_combo->currentIndex());
    settings.set_deepl_api_key(m_deepl_key_edit->text().toStdString());
    settings.set_google_api_key(m_google_key_edit->text().toStdString());
}

void translation_settings_view_t::on_provider_changed(int index)
{
    const bool deepl_selected = (index == 1);
    const bool google_selected = (index == 2);

    m_deepl_key_edit->setEnabled(deepl_selected);
    m_deepl_reveal_button->setEnabled(deepl_selected);
    m_google_key_edit->setEnabled(google_selected);
    m_google_reveal_button->setEnabled(google_selected);
}
