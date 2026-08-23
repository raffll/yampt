#include "merge_settings_view.hpp"
#include <settings_store.hpp>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QVBoxLayout>

merge_settings_view_t::merge_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(0, 0, 0, 0);

	m_tabs = new QTabWidget(this);
	main_layout->addWidget(m_tabs);

	setup_exclude_tab();
	setup_fixes_tab();
}

void merge_settings_view_t::setup_exclude_tab()
{
	auto * page = new QWidget(m_tabs);
	auto * page_layout = new QVBoxLayout(page);

	auto * ignore_group = new QGroupBox(tr("Ignore Fields"), page);
	auto * ignore_layout = new QVBoxLayout(ignore_group);

	auto * ignore_desc = new QLabel(
	    tr("Sub-records listed here are hidden from conflict detection and excluded from the merged patch.\n"
	       "Use TYPE:* to ignore an entire record type (e.g. CELL:* skips all cell records)."),
	    ignore_group);
	ignore_desc->setWordWrap(true);
	ignore_layout->addWidget(ignore_desc);

	m_ignore_fields_edit = new QLineEdit(ignore_group);
	m_ignore_fields_edit->setMinimumHeight(32);
	m_ignore_fields_edit->setToolTip(tr("Comma-separated list of RECORD:SUB entries"));
	m_ignore_fields_edit->setPlaceholderText(tr("CELL:NAM0, NPC_:AI_W, ARMO:*"));
	ignore_layout->addWidget(m_ignore_fields_edit);

	page_layout->addWidget(ignore_group);

	auto * exclusion_group = new QGroupBox(tr("Exclude by ID"), page);
	auto * exclusion_layout = new QVBoxLayout(exclusion_group);

	auto * exclusion_desc = new QLabel(
	    tr("Records whose ID matches this regular expression are skipped entirely during auto-merge.\n"
	       "Use this to exclude specific named records regardless of their type."),
	    exclusion_group);
	exclusion_desc->setWordWrap(true);
	exclusion_layout->addWidget(exclusion_desc);

	m_exclusion_edit = new QLineEdit(exclusion_group);
	m_exclusion_edit->setMinimumHeight(32);
	m_exclusion_edit->setToolTip(tr("Regex matched against record IDs"));
	m_exclusion_edit->setPlaceholderText(tr("^MyMod_.* | ^TR_.* | balmora"));
	exclusion_layout->addWidget(m_exclusion_edit);

	page_layout->addWidget(exclusion_group);
	page_layout->addStretch();

	m_tabs->addTab(page, tr("Exclude"));
}

void merge_settings_view_t::setup_fixes_tab()
{
	auto * page = new QWidget(m_tabs);
	auto * page_layout = new QVBoxLayout(page);

	auto * group = new QGroupBox(tr("Bug Fixes"), page);
	auto * fixes_layout = new QVBoxLayout(group);

	m_fog_fix_check = new QCheckBox(tr("Fix fog density"), group);
	m_fog_fix_check->setChecked(true);
	m_fog_fix_check->setToolTip(tr("Fix zero fog density in interior cells"));
	fixes_layout->addWidget(m_fog_fix_check);

	m_summon_fix_check = new QCheckBox(tr("Fix summon persistence"), group);
	m_summon_fix_check->setChecked(true);
	m_summon_fix_check->setToolTip(tr("Add persistent flag to summoned creatures"));
	fixes_layout->addWidget(m_summon_fix_check);

	m_cell_name_fix_check = new QCheckBox(tr("Fix cell name reversion"), group);
	m_cell_name_fix_check->setChecked(true);
	m_cell_name_fix_check->setToolTip(tr("Prevent cell name reversions by later plugins"));
	fixes_layout->addWidget(m_cell_name_fix_check);

	page_layout->addWidget(group);
	page_layout->addStretch();

	m_tabs->addTab(page, tr("Fixes"));
}

void merge_settings_view_t::load(const settings_store_t & settings)
{
	m_ignore_fields_edit->setText(QString::fromStdString(settings.sub_record_ignore_conflict()));
	m_exclusion_edit->setText(QString::fromStdString(settings.merge_exclusion_pattern()));
	m_fog_fix_check->setChecked(settings.merge_fog_fix_enabled());
	m_summon_fix_check->setChecked(settings.merge_summon_fix_enabled());
	m_cell_name_fix_check->setChecked(settings.merge_cell_name_fix_enabled());
}

void merge_settings_view_t::save(settings_store_t & settings) const
{
	settings.set_sub_record_ignore_conflict(m_ignore_fields_edit->text().toStdString());
	settings.set_merge_exclusion_pattern(m_exclusion_edit->text().toStdString());
	settings.set_merge_fog_fix_enabled(m_fog_fix_check->isChecked());
	settings.set_merge_summon_fix_enabled(m_summon_fix_check->isChecked());
	settings.set_merge_cell_name_fix_enabled(m_cell_name_fix_check->isChecked());
}
