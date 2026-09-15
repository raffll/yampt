#include "merge_settings_view.hpp"
#include <scanner/merge_exclusions.hpp>
#include <settings_store.hpp>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {
constexpr int kind_column = 0;
constexpr int target_column = 1;

exclude_kind_t kind_for_index(int index)
{
	switch (index)
	{
	case 0:
		return exclude_kind_t::file;

	case 2:
		return exclude_kind_t::record_type;

	case 3:
		return exclude_kind_t::sub_record;

	default:
		return exclude_kind_t::record_id;
	}
}

int index_for_kind(exclude_kind_t kind)
{
	switch (kind)
	{
	case exclude_kind_t::file:
		return 0;

	case exclude_kind_t::record_type:
		return 2;

	case exclude_kind_t::sub_record:
		return 3;

	case exclude_kind_t::record_id:
		return 1;
	}

	return 1;
}

QString kind_label(exclude_kind_t kind)
{
	switch (kind)
	{
	case exclude_kind_t::file:
		return QCoreApplication::translate("yEditor", "File");

	case exclude_kind_t::record_type:
		return QCoreApplication::translate("yEditor", "Record Type");

	case exclude_kind_t::sub_record:
		return QCoreApplication::translate("yEditor", "Sub-Record");

	case exclude_kind_t::record_id:
		return QCoreApplication::translate("yEditor", "Record ID");
	}

	return QCoreApplication::translate("yEditor", "Record ID");
}

QString placeholder_for_index(int index)
{
	switch (kind_for_index(index))
	{
	case exclude_kind_t::file:
		return QCoreApplication::translate("yEditor", "SomeMod.esp");

	case exclude_kind_t::record_type:
		return QCoreApplication::translate("yEditor", "REGN");

	case exclude_kind_t::sub_record:
		return QCoreApplication::translate("yEditor", "CELL:NAM0");

	case exclude_kind_t::record_id:
		return QCoreApplication::translate("yEditor", "^MyMod_.*");
	}

	return {};
}
} // namespace

merge_settings_view_t::merge_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(0, 0, 0, 0);

	m_tabs = new QTabWidget(this);
	main_layout->addWidget(m_tabs);

	setup_excludes_tab();
	setup_fixes_tab();
}

void merge_settings_view_t::setup_excludes_tab()
{
	auto * page = new QWidget(m_tabs);
	auto * page_layout = new QVBoxLayout(page);
	page_layout->setContentsMargins(2, 2, 2, 2);

	auto * desc = new QLabel(
	    tr("Content listed here is left out of the merged patch. Choose what each rule targets: an entire plugin "
	       "file, a record ID (regular expression), a whole record type, or a single sub-record (TYPE:SUB)."),
	    page);
	desc->setWordWrap(true);
	page_layout->addWidget(desc);

	m_exclude_table = new QTableWidget(0, 2, page);
	m_exclude_table->setHorizontalHeaderLabels({ tr("Kind"), tr("Target") });
	m_exclude_table->horizontalHeader()->setSectionResizeMode(kind_column, QHeaderView::ResizeToContents);
	m_exclude_table->horizontalHeader()->setSectionResizeMode(target_column, QHeaderView::Stretch);
	m_exclude_table->verticalHeader()->setVisible(false);
	m_exclude_table->verticalHeader()->setDefaultSectionSize(24);
	m_exclude_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_exclude_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_exclude_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	page_layout->addWidget(m_exclude_table, 1);

	auto * input_row = new QHBoxLayout;

	m_kind_combo = new QComboBox(page);
	m_kind_combo->addItem(kind_label(exclude_kind_t::file));
	m_kind_combo->addItem(kind_label(exclude_kind_t::record_id));
	m_kind_combo->addItem(kind_label(exclude_kind_t::record_type));
	m_kind_combo->addItem(kind_label(exclude_kind_t::sub_record));
	m_kind_combo->setCurrentIndex(1);
	m_kind_combo->setToolTip(tr("What the exclusion rule targets"));
	input_row->addWidget(m_kind_combo);

	m_target_input = new QLineEdit(page);
	m_target_input->setPlaceholderText(placeholder_for_index(m_kind_combo->currentIndex()));
	m_target_input->setToolTip(tr("Value to exclude for the selected kind"));
	input_row->addWidget(m_target_input, 1);

	m_exclude_add_button = new QPushButton(tr("Add"), page);
	m_exclude_add_button->setToolTip(tr("Add exclusion rule"));
	input_row->addWidget(m_exclude_add_button);

	m_exclude_remove_button = new QPushButton(tr("Remove"), page);
	m_exclude_remove_button->setToolTip(tr("Remove selected rule"));
	input_row->addWidget(m_exclude_remove_button);

	page_layout->addLayout(input_row);

	connect(
	    m_kind_combo,
	    &QComboBox::currentIndexChanged,
	    this,
	    [this](int index) { m_target_input->setPlaceholderText(placeholder_for_index(index)); });
	connect(m_exclude_add_button, &QPushButton::clicked, this, &merge_settings_view_t::on_exclude_add);
	connect(m_exclude_remove_button, &QPushButton::clicked, this, &merge_settings_view_t::on_exclude_remove);
	connect(m_target_input, &QLineEdit::returnPressed, this, &merge_settings_view_t::on_exclude_add);

	m_tabs->addTab(page, tr("Excludes"));
}

void merge_settings_view_t::setup_fixes_tab()
{
	auto * page = new QWidget(m_tabs);
	auto * page_layout = new QVBoxLayout(page);
	page_layout->setContentsMargins(2, 2, 2, 2);

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

bool merge_settings_view_t::row_exists(int kind_index, const QString & target) const
{
	for (int row = 0; row < m_exclude_table->rowCount(); ++row)
	{
		const auto * kind_item = m_exclude_table->item(row, kind_column);
		const auto * target_item = m_exclude_table->item(row, target_column);
		if (!kind_item || !target_item)
			continue;

		if (kind_item->data(Qt::UserRole).toInt() == kind_index && target_item->text() == target)
			return true;
	}

	return false;
}

void merge_settings_view_t::on_exclude_add()
{
	const auto target = m_target_input->text().trimmed();
	if (target.isEmpty())
		return;

	const int kind_index = m_kind_combo->currentIndex();
	if (row_exists(kind_index, target))
		return;

	const int row = m_exclude_table->rowCount();
	m_exclude_table->insertRow(row);

	auto * kind_item = new QTableWidgetItem(m_kind_combo->itemText(kind_index));
	kind_item->setData(Qt::UserRole, kind_index);
	m_exclude_table->setItem(row, kind_column, kind_item);
	m_exclude_table->setItem(row, target_column, new QTableWidgetItem(target));

	m_target_input->clear();
}

void merge_settings_view_t::on_exclude_remove()
{
	const int row = m_exclude_table->currentRow();
	if (row < 0)
		return;

	m_exclude_table->removeRow(row);
}

void merge_settings_view_t::load(const settings_store_t & settings)
{
	m_exclude_table->setRowCount(0);

	const auto rules = merge_exclusions_t::parse(settings.merge_excludes());
	for (const auto & rule : rules)
	{
		const int kind_index = index_for_kind(rule.kind);
		const int row = m_exclude_table->rowCount();
		m_exclude_table->insertRow(row);

		auto * kind_item = new QTableWidgetItem(kind_label(rule.kind));
		kind_item->setData(Qt::UserRole, kind_index);
		m_exclude_table->setItem(row, kind_column, kind_item);
		m_exclude_table->setItem(row, target_column, new QTableWidgetItem(QString::fromStdString(rule.target)));
	}

	m_fog_fix_check->setChecked(settings.merge_fog_fix_enabled());
	m_summon_fix_check->setChecked(settings.merge_summon_fix_enabled());
	m_cell_name_fix_check->setChecked(settings.merge_cell_name_fix_enabled());
}

void merge_settings_view_t::save(settings_store_t & settings) const
{
	std::vector<exclude_rule_t> rules;
	for (int row = 0; row < m_exclude_table->rowCount(); ++row)
	{
		const auto * kind_item = m_exclude_table->item(row, kind_column);
		const auto * target_item = m_exclude_table->item(row, target_column);
		if (!kind_item || !target_item)
			continue;

		rules.push_back({ kind_for_index(kind_item->data(Qt::UserRole).toInt()), target_item->text().toStdString() });
	}

	settings.set_merge_excludes(merge_exclusions_t::serialize(rules));

	settings.set_merge_fog_fix_enabled(m_fog_fix_check->isChecked());
	settings.set_merge_summon_fix_enabled(m_summon_fix_check->isChecked());
	settings.set_merge_cell_name_fix_enabled(m_cell_name_fix_check->isChecked());
}
