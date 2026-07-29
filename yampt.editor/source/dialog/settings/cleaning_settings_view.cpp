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

	layout->addWidget(group);

	auto * header_group = new QGroupBox(tr("Header Repair"), this);
	auto * header_layout = new QVBoxLayout(header_group);

	m_update_master_sizes_check = new QCheckBox(tr("Update master file sizes"), header_group);
	m_update_master_sizes_check->setToolTip(
	    tr("Set master file sizes in the header to match actual file sizes on disk"));
	header_layout->addWidget(m_update_master_sizes_check);

	m_update_version_check = new QCheckBox(tr("Update plugin version to 1.3"), header_group);
	m_update_version_check->setToolTip(tr("Set the HEDR version field to 1.3 (required by some engines)"));
	header_layout->addWidget(m_update_version_check);

	layout->addWidget(header_group);
	layout->addStretch();
}

void cleaning_settings_view_t::load(const settings_store_t & settings)
{
	m_evil_gmst_check->setChecked(settings.clean_evil_gmst_enabled());
	m_junk_cell_check->setChecked(settings.clean_junk_cell_enabled());
	m_update_master_sizes_check->setChecked(settings.clean_update_master_sizes());
	m_update_version_check->setChecked(settings.clean_update_version());
}

void cleaning_settings_view_t::save(settings_store_t & settings) const
{
	settings.set_clean_evil_gmst_enabled(m_evil_gmst_check->isChecked());
	settings.set_clean_junk_cell_enabled(m_junk_cell_check->isChecked());
	settings.set_clean_update_master_sizes(m_update_master_sizes_check->isChecked());
	settings.set_clean_update_version(m_update_version_check->isChecked());
}
