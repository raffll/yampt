#include "cleaning_settings_view.hpp"
#include <settings_store.hpp>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

cleaning_settings_view_t::cleaning_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);

	auto * group = new QGroupBox(tr("Plugin Cleaning"), this);
	auto * group_layout = new QVBoxLayout(group);

	m_evil_gmst_check = new QCheckBox(tr("Remove evil GMSTs"), group);
	m_evil_gmst_check->setToolTip(tr("Remove Tribunal/Bloodmoon GMSTs injected by the Construction Set"));
	group_layout->addWidget(m_evil_gmst_check);

	m_junk_cell_check = new QCheckBox(tr("Remove junk cells"), group);
	m_junk_cell_check->setToolTip(tr("Remove exterior cells that contain only NAME and DATA sub-records"));
	group_layout->addWidget(m_junk_cell_check);

	m_itm_check = new QCheckBox(tr("Remove ITM records"), group);
	m_itm_check->setToolTip(tr("Not yet available — ITM detection needs further testing"));
	m_itm_check->setEnabled(false);
	m_itm_check->setChecked(false);
	group_layout->addWidget(m_itm_check);

	layout->addWidget(group);
	layout->addStretch();
}

void cleaning_settings_view_t::load(const settings_store_t & settings)
{
	m_evil_gmst_check->setChecked(settings.clean_evil_gmst_enabled());
	m_junk_cell_check->setChecked(settings.clean_junk_cell_enabled());
}

void cleaning_settings_view_t::save(settings_store_t & settings) const
{
	settings.set_clean_evil_gmst_enabled(m_evil_gmst_check->isChecked());
	settings.set_clean_junk_cell_enabled(m_junk_cell_check->isChecked());
}
