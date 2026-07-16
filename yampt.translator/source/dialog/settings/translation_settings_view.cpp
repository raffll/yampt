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

	m_claude_key_edit = new QLineEdit(this);
	m_claude_key_edit->setEchoMode(QLineEdit::Password);
	m_claude_key_edit->setPlaceholderText("sk-ant-...");
	form->addRow("Claude API Key:", m_claude_key_edit);

	layout->addLayout(form);
	layout->addStretch();
}

void translation_settings_view_t::load(const settings_store_t & settings)
{
	m_claude_key_edit->setText(QString::fromStdString(settings.claude_api_key()));
}

void translation_settings_view_t::apply(settings_store_t & settings) const
{
	settings.set_claude_api_key(m_claude_key_edit->text().toStdString());
}
