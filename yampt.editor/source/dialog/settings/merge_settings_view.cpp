#include "merge_settings_view.hpp"
#include <settings_store.hpp>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
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
	setup_record_types_group();
	setup_exclusion_group();
	setup_fixes_group();
	main_layout->addStretch();
}

void merge_settings_view_t::setup_record_types_group()
{
	auto * parent_layout = qobject_cast<QVBoxLayout *>(layout());
	auto * group = new QGroupBox(tr("Record Types"), this);
	auto * grid = new QGridLayout(group);

	for (int i = 0; i < record_type_count; ++i)
	{
		const auto * type_name = record_types[i];
		auto * checkbox = new QCheckBox(type_name, group);
		checkbox->setChecked(true);
		checkbox->setToolTip(QString("Include %1 records in merged patch").arg(type_name));
		grid->addWidget(checkbox, i / columns, i % columns);
		m_type_checkboxes[type_name] = checkbox;
	}

	parent_layout->addWidget(group);
}

void merge_settings_view_t::setup_exclusion_group()
{
	auto * parent_layout = qobject_cast<QVBoxLayout *>(layout());

	auto * label = new QLabel(tr("Exclusion pattern (regex):"), this);
	parent_layout->addWidget(label);

	m_exclusion_edit = new QLineEdit(this);
	m_exclusion_edit->setToolTip(tr("Records matching this regex are excluded from merged patch"));
	parent_layout->addWidget(m_exclusion_edit);
}

void merge_settings_view_t::setup_fixes_group()
{
	auto * parent_layout = qobject_cast<QVBoxLayout *>(layout());
	auto * group = new QGroupBox(tr("Bug Fixes"), this);
	auto * fixes_layout = new QVBoxLayout(group);

	m_fog_fix_check = new QCheckBox("Fix fog density", group);
	m_fog_fix_check->setChecked(true);
	m_fog_fix_check->setToolTip(tr("Fix zero fog density in interior cells"));
	fixes_layout->addWidget(m_fog_fix_check);

	m_summon_fix_check = new QCheckBox("Fix summon persistence", group);
	m_summon_fix_check->setChecked(true);
	m_summon_fix_check->setToolTip(tr("Add persistent flag to summoned creatures"));
	fixes_layout->addWidget(m_summon_fix_check);

	m_cell_name_fix_check = new QCheckBox("Fix cell name reversion", group);
	m_cell_name_fix_check->setChecked(true);
	m_cell_name_fix_check->setToolTip(tr("Prevent cell name reversions by later plugins"));
	fixes_layout->addWidget(m_cell_name_fix_check);

	parent_layout->addWidget(group);
}

void merge_settings_view_t::load(const settings_store_t & settings)
{
	for (auto & [type_name, checkbox] : m_type_checkboxes)
		checkbox->setChecked(settings.merge_type_enabled(type_name));

	m_exclusion_edit->setText(QString::fromStdString(settings.merge_exclusion_pattern()));
	m_fog_fix_check->setChecked(settings.merge_fog_fix_enabled());
	m_summon_fix_check->setChecked(settings.merge_summon_fix_enabled());
	m_cell_name_fix_check->setChecked(settings.merge_cell_name_fix_enabled());
}

void merge_settings_view_t::save(settings_store_t & settings) const
{
	for (const auto & [type_name, checkbox] : m_type_checkboxes)
		settings.set_merge_type_enabled(type_name, checkbox->isChecked());

	settings.set_merge_exclusion_pattern(m_exclusion_edit->text().toStdString());
	settings.set_merge_fog_fix_enabled(m_fog_fix_check->isChecked());
	settings.set_merge_summon_fix_enabled(m_summon_fix_check->isChecked());
	settings.set_merge_cell_name_fix_enabled(m_cell_name_fix_check->isChecked());
}
