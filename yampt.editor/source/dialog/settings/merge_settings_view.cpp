#include "merge_settings_view.hpp"
#include <settings_store.hpp>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPalette>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

merge_settings_view_t::merge_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(0, 0, 0, 0);

	m_tabs = new QTabWidget(this);
	main_layout->addWidget(m_tabs);

	setup_ignore_fields_tab();
	setup_exclude_by_id_tab();
	setup_fixes_tab();
}

void merge_settings_view_t::setup_ignore_fields_tab()
{
	auto * page = new QWidget(m_tabs);
	auto * page_layout = new QVBoxLayout(page);

	auto * desc = new QLabel(
	    tr("Sub-records listed here are hidden from conflict detection and excluded from the merged patch. "
	       "Use TYPE:* to ignore an entire record type (e.g. CELL:* skips all cell records)."),
	    page);
	desc->setWordWrap(true);
	auto ignore_palette = desc->palette();
	ignore_palette.setColor(QPalette::WindowText, ignore_palette.color(QPalette::Disabled, QPalette::WindowText));
	desc->setPalette(ignore_palette);
	page_layout->addWidget(desc);

	m_ignore_list = new QListWidget(page);
	page_layout->addWidget(m_ignore_list, 1);

	auto * input_row = new QHBoxLayout;

	m_ignore_input = new QLineEdit(page);
	m_ignore_input->setPlaceholderText(tr("CELL:NAM0"));
	m_ignore_input->setToolTip(tr("Enter a RECORD:SUB entry to add"));
	input_row->addWidget(m_ignore_input, 1);

	m_ignore_add_button = new QPushButton(tr("Add"), page);
	m_ignore_add_button->setToolTip(tr("Add entry to ignore list"));
	input_row->addWidget(m_ignore_add_button);

	m_ignore_remove_button = new QPushButton(tr("Remove"), page);
	m_ignore_remove_button->setToolTip(tr("Remove selected entry"));
	input_row->addWidget(m_ignore_remove_button);

	page_layout->addLayout(input_row);

	connect(m_ignore_add_button, &QPushButton::clicked, this, &merge_settings_view_t::on_ignore_add);
	connect(m_ignore_remove_button, &QPushButton::clicked, this, &merge_settings_view_t::on_ignore_remove);
	connect(m_ignore_input, &QLineEdit::returnPressed, this, &merge_settings_view_t::on_ignore_add);

	m_tabs->addTab(page, tr("Exclude Sub-Records"));
}

void merge_settings_view_t::setup_exclude_by_id_tab()
{
	auto * page = new QWidget(m_tabs);
	auto * page_layout = new QVBoxLayout(page);

	auto * desc = new QLabel(
	    tr("Records whose ID matches any of these regular expressions are skipped entirely during auto-merge. "
	       "Use this to exclude specific named records regardless of their type."),
	    page);
	desc->setWordWrap(true);
	auto exclude_palette = desc->palette();
	exclude_palette.setColor(QPalette::WindowText, exclude_palette.color(QPalette::Disabled, QPalette::WindowText));
	desc->setPalette(exclude_palette);
	page_layout->addWidget(desc);

	m_exclude_list = new QListWidget(page);
	page_layout->addWidget(m_exclude_list, 1);

	auto * input_row = new QHBoxLayout;

	m_exclude_input = new QLineEdit(page);
	m_exclude_input->setPlaceholderText(tr("^MyMod_.*"));
	m_exclude_input->setToolTip(tr("Enter a regex pattern to add"));
	input_row->addWidget(m_exclude_input, 1);

	m_exclude_add_button = new QPushButton(tr("Add"), page);
	m_exclude_add_button->setToolTip(tr("Add pattern to exclusion list"));
	input_row->addWidget(m_exclude_add_button);

	m_exclude_remove_button = new QPushButton(tr("Remove"), page);
	m_exclude_remove_button->setToolTip(tr("Remove selected pattern"));
	input_row->addWidget(m_exclude_remove_button);

	page_layout->addLayout(input_row);

	connect(m_exclude_add_button, &QPushButton::clicked, this, &merge_settings_view_t::on_exclude_add);
	connect(m_exclude_remove_button, &QPushButton::clicked, this, &merge_settings_view_t::on_exclude_remove);
	connect(m_exclude_input, &QLineEdit::returnPressed, this, &merge_settings_view_t::on_exclude_add);

	m_tabs->addTab(page, tr("Exclude by ID"));
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

void merge_settings_view_t::on_ignore_add()
{
	const auto text = m_ignore_input->text().trimmed();
	if (text.isEmpty())
		return;

	for (int i = 0; i < m_ignore_list->count(); ++i)
	{
		if (m_ignore_list->item(i)->text() == text)
			return;
	}

	m_ignore_list->addItem(text);
	m_ignore_input->clear();
}

void merge_settings_view_t::on_ignore_remove()
{
	const auto * item = m_ignore_list->currentItem();
	if (!item)
		return;

	delete m_ignore_list->takeItem(m_ignore_list->row(item));
}

void merge_settings_view_t::on_exclude_add()
{
	const auto text = m_exclude_input->text().trimmed();
	if (text.isEmpty())
		return;

	for (int i = 0; i < m_exclude_list->count(); ++i)
	{
		if (m_exclude_list->item(i)->text() == text)
			return;
	}

	m_exclude_list->addItem(text);
	m_exclude_input->clear();
}

void merge_settings_view_t::on_exclude_remove()
{
	const auto * item = m_exclude_list->currentItem();
	if (!item)
		return;

	delete m_exclude_list->takeItem(m_exclude_list->row(item));
}

void merge_settings_view_t::load(const settings_store_t & settings)
{
	m_ignore_list->clear();
	const auto ignore_str = QString::fromStdString(settings.sub_record_ignore_conflict());
	const auto ignore_parts = ignore_str.split(',', Qt::SkipEmptyParts);
	for (const auto & part : ignore_parts)
	{
		const auto trimmed = part.trimmed();
		if (!trimmed.isEmpty())
			m_ignore_list->addItem(trimmed);
	}

	m_exclude_list->clear();
	const auto exclude_str = QString::fromStdString(settings.merge_exclusion_pattern());
	const auto exclude_parts = exclude_str.split(',', Qt::SkipEmptyParts);
	for (const auto & part : exclude_parts)
	{
		const auto trimmed = part.trimmed();
		if (!trimmed.isEmpty())
			m_exclude_list->addItem(trimmed);
	}

	m_fog_fix_check->setChecked(settings.merge_fog_fix_enabled());
	m_summon_fix_check->setChecked(settings.merge_summon_fix_enabled());
	m_cell_name_fix_check->setChecked(settings.merge_cell_name_fix_enabled());
}

void merge_settings_view_t::save(settings_store_t & settings) const
{
	QStringList ignore_entries;
	for (int i = 0; i < m_ignore_list->count(); ++i)
		ignore_entries.append(m_ignore_list->item(i)->text());

	settings.set_sub_record_ignore_conflict(ignore_entries.join(", ").toStdString());

	QStringList exclude_entries;
	for (int i = 0; i < m_exclude_list->count(); ++i)
		exclude_entries.append(m_exclude_list->item(i)->text());

	settings.set_merge_exclusion_pattern(exclude_entries.join("|").toStdString());

	settings.set_merge_fog_fix_enabled(m_fog_fix_check->isChecked());
	settings.set_merge_summon_fix_enabled(m_summon_fix_check->isChecked());
	settings.set_merge_cell_name_fix_enabled(m_cell_name_fix_check->isChecked());
}
