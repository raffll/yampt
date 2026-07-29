#include "sub_record_rules_view.hpp"
#include <settings_store.hpp>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

sub_record_rules_view_t::sub_record_rules_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);

	auto * group = new QGroupBox(tr("Sub-Record Rules"), this);
	auto * form = new QFormLayout(group);

	m_ignore_conflict_edit = new QLineEdit(group);
	m_ignore_conflict_edit->setToolTip(tr("Sub-records that should not be flagged as conflicts"));
	form->addRow(tr("Ignore Conflict:"), m_ignore_conflict_edit);

	m_exclude_from_merge_edit = new QLineEdit(group);
	m_exclude_from_merge_edit->setToolTip(tr("Sub-records excluded from the merged patch output"));
	form->addRow(tr("Exclude from Merge:"), m_exclude_from_merge_edit);

	m_skip_if_missing_edit = new QLineEdit(group);
	m_skip_if_missing_edit->setToolTip(tr("Sub-records ignored when not present in a plugin"));
	form->addRow(tr("Skip if Missing:"), m_skip_if_missing_edit);

	layout->addWidget(group);

	auto * hint = new QLabel(tr("Format: RECORD:SUB, RECORD:SUB  (e.g. CELL:NAM0, CELL:NAM9, CELL:*)"), this);
	hint->setStyleSheet("color: #888; font-size: 11px;");
	layout->addWidget(hint);

	layout->addStretch();
}

void sub_record_rules_view_t::load(const settings_store_t & settings)
{
	m_ignore_conflict_edit->setText(QString::fromStdString(settings.sub_record_ignore_conflict()));
	m_exclude_from_merge_edit->setText(QString::fromStdString(settings.sub_record_exclude_from_merge()));
	m_skip_if_missing_edit->setText(QString::fromStdString(settings.sub_record_skip_if_missing()));
}

void sub_record_rules_view_t::save(settings_store_t & settings) const
{
	settings.set_sub_record_ignore_conflict(m_ignore_conflict_edit->text().toStdString());
	settings.set_sub_record_exclude_from_merge(m_exclude_from_merge_edit->text().toStdString());
	settings.set_sub_record_skip_if_missing(m_skip_if_missing_edit->text().toStdString());
}
