#include "merge_settings_view.hpp"
#include <settings_store.hpp>
#include <QCheckBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

constexpr int columns = 5;

const char * record_types[] = { "ACTI", "ALCH", "APPA", "ARMO", "BODY", "BOOK", "BSGN", "CELL", "CLAS",
	                            "CLOT", "CONT", "CREA", "DIAL", "DOOR", "ENCH", "FACT", "GLOB", "GMST",
	                            "INGR", "LAND", "LEVC", "LEVI", "LIGH", "LOCK", "MGEF", "MISC", "NPC_",
	                            "PGRD", "REGN", "SCPT", "SKIL", "SNDG", "SOUN", "SPEL", "STAT", "WEAP" };

constexpr int record_type_count = sizeof(record_types) / sizeof(record_types[0]);

} // namespace

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

	auto * types_group = new QGroupBox(tr("Record Types"), page);
	auto * grid = new QGridLayout(types_group);

	for (int i = 0; i < record_type_count; ++i)
	{
		const auto * type_name = record_types[i];
		auto * checkbox = new QCheckBox(type_name, types_group);
		checkbox->setChecked(true);
		checkbox->setToolTip(tr("Include %1 records in merged patch").arg(type_name));
		grid->addWidget(checkbox, i / columns, i % columns);
		m_type_checkboxes[type_name] = checkbox;
	}

	page_layout->addWidget(types_group);

	auto * exclusion_group = new QGroupBox(tr("Exclusion Pattern"), page);
	auto * exclusion_form = new QFormLayout(exclusion_group);

	m_exclusion_edit = new QLineEdit(exclusion_group);
	m_exclusion_edit->setToolTip(tr("Records matching this regex are excluded from merged patch"));
	exclusion_form->addRow(tr("Regex:"), m_exclusion_edit);

	page_layout->addWidget(exclusion_group);

	auto * ignore_group = new QGroupBox(tr("Ignore Sub-Records"), page);
	auto * ignore_form = new QFormLayout(ignore_group);

	m_ignore_sub_records_edit = new QLineEdit(ignore_group);
	m_ignore_sub_records_edit->setToolTip(
	    tr("Sub-records excluded from conflict detection and merged patch output"));
	ignore_form->addRow(tr("Ignore:"), m_ignore_sub_records_edit);

	auto * hint = new QLabel(
	    tr("Format: RECORD:SUB, RECORD:SUB  (e.g. CELL:NAM0, NPC_:AI_W, ARMO:*)"), ignore_group);
	hint->setStyleSheet("color: #888; font-size: 11px;");
	ignore_form->addRow(hint);

	page_layout->addWidget(ignore_group);
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
	for (auto & [type_name, checkbox] : m_type_checkboxes)
		checkbox->setChecked(settings.merge_type_enabled(type_name));

	m_exclusion_edit->setText(QString::fromStdString(settings.merge_exclusion_pattern()));
	m_fog_fix_check->setChecked(settings.merge_fog_fix_enabled());
	m_summon_fix_check->setChecked(settings.merge_summon_fix_enabled());
	m_cell_name_fix_check->setChecked(settings.merge_cell_name_fix_enabled());
	m_ignore_sub_records_edit->setText(QString::fromStdString(settings.sub_record_ignore_conflict()));
}

void merge_settings_view_t::save(settings_store_t & settings) const
{
	for (const auto & [type_name, checkbox] : m_type_checkboxes)
		settings.set_merge_type_enabled(type_name, checkbox->isChecked());

	settings.set_merge_exclusion_pattern(m_exclusion_edit->text().toStdString());
	settings.set_merge_fog_fix_enabled(m_fog_fix_check->isChecked());
	settings.set_merge_summon_fix_enabled(m_summon_fix_check->isChecked());
	settings.set_merge_cell_name_fix_enabled(m_cell_name_fix_check->isChecked());
	settings.set_sub_record_ignore_conflict(m_ignore_sub_records_edit->text().toStdString());
}
