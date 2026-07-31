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

	m_ignore_edit = new QLineEdit(group);
	m_ignore_edit->setToolTip(tr("Sub-records excluded from conflict detection and merged patch output"));
	form->addRow(tr("Ignore Sub-Records:"), m_ignore_edit);

	layout->addWidget(group);

	auto * hint = new QLabel(tr("Format: RECORD:SUB, RECORD:SUB  (e.g. CELL:NAM0, NPC_:AI_W, ARMO:*)"), this);
	hint->setStyleSheet("color: #888; font-size: 11px;");
	layout->addWidget(hint);

	layout->addStretch();
}

void sub_record_rules_view_t::load(const settings_store_t & settings)
{
	m_ignore_edit->setText(QString::fromStdString(settings.sub_record_ignore_conflict()));
}

void sub_record_rules_view_t::save(settings_store_t & settings) const
{
	settings.set_sub_record_ignore_conflict(m_ignore_edit->text().toStdString());
}
