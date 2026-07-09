#include "translation_settings_view.hpp"
#include <settings_store.hpp>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

translation_settings_view_t::translation_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	auto * form = new QFormLayout;

	m_deepl_key_edit = new QLineEdit(this);
	m_deepl_key_edit->setEchoMode(QLineEdit::Password);
	form->addRow("DeepL API Key:", m_deepl_key_edit);

	m_google_key_edit = new QLineEdit(this);
	m_google_key_edit->setEchoMode(QLineEdit::Password);
	form->addRow("Google API Key:", m_google_key_edit);

	layout->addLayout(form);
	layout->addStretch();
}

void translation_settings_view_t::load(const settings_store_t & settings)
{
	m_deepl_key_edit->setText(QString::fromStdString(settings.deepl_api_key()));
	m_google_key_edit->setText(QString::fromStdString(settings.google_api_key()));
}

void translation_settings_view_t::apply(settings_store_t & settings) const
{
	settings.set_deepl_api_key(m_deepl_key_edit->text().toStdString());
	settings.set_google_api_key(m_google_key_edit->text().toStdString());
}
