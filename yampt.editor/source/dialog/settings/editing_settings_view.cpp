#include "editing_settings_view.hpp"
#include <settings_store.hpp>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

editing_settings_view_t::editing_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);

	auto * group = new QGroupBox(tr("Direct Editing"), this);
	auto * group_layout = new QVBoxLayout(group);

	m_editing_check = new QCheckBox(tr("Allow editing decoded fields in loaded plugins"), group);
	m_editing_check->setToolTip(tr("Enable in-place editing of fields in every loaded plugin"));
	group_layout->addWidget(m_editing_check);

	auto * warning = new QLabel(
	    tr("Editing fields directly in a plugin rewrites that plugin on save and can break it if a value is "
	       "malformed. Only enable this if you know what you are doing. The merged patch is always editable "
	       "regardless of this setting."),
	    group);
	warning->setWordWrap(true);
	warning->setStyleSheet("color: #b00020;");
	group_layout->addWidget(warning);

	layout->addWidget(group);
	layout->addStretch();
}

void editing_settings_view_t::load(const settings_store_t & settings)
{
	m_editing_check->setChecked(settings.editing_enabled());
}

void editing_settings_view_t::save(settings_store_t & settings) const
{
	settings.set_editing_enabled(m_editing_check->isChecked());
}
