#include "find_replace_dialog.hpp"
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

find_replace_dialog_t::find_replace_dialog_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QGridLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	setup_layout(layout);
	connect_signals();
}

void find_replace_dialog_t::setup_layout(QGridLayout * layout)
{
	layout->addWidget(new QLabel(tr("Find:"), this), 0, 0);
	m_find_field = new QLineEdit(this);
	layout->addWidget(m_find_field, 0, 1, 1, 3);

	layout->addWidget(new QLabel(tr("Replace:"), this), 1, 0);
	m_replace_field = new QLineEdit(this);
	layout->addWidget(m_replace_field, 1, 1, 1, 3);

	m_case_check = new QCheckBox("Case sensitive", this);
	layout->addWidget(m_case_check, 2, 0, 1, 2);

	m_regex_check = new QCheckBox("Regex", this);
	layout->addWidget(m_regex_check, 2, 2, 1, 2);

	m_find_next_btn = new QPushButton(tr("Find Next"), this);
	m_find_next_btn->setToolTip(tr("Find next matching entry"));
	layout->addWidget(m_find_next_btn, 3, 1);

	m_replace_btn = new QPushButton(tr("Replace"), this);
	m_replace_btn->setToolTip(tr("Replace current match and find next"));
	layout->addWidget(m_replace_btn, 3, 2);

	m_replace_all_btn = new QPushButton(tr("Replace All"), this);
	m_replace_all_btn->setToolTip(tr("Replace in all entries regardless of filters"));
	layout->addWidget(m_replace_all_btn, 3, 3);

	m_note_label = new QLabel(tr("Replace All affects all entries regardless of filters."), this);
	m_note_label->setStyleSheet("color: #888; font-style: italic;");
	layout->addWidget(m_note_label, 4, 0, 1, 4);
}

void find_replace_dialog_t::connect_signals()
{
	connect(
	    m_find_next_btn,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{ emit find_next_requested(m_find_field->text(), m_case_check->isChecked(), m_regex_check->isChecked()); });

	connect(
	    m_replace_btn,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		emit replace_requested(
		    m_find_field->text(), m_replace_field->text(), m_case_check->isChecked(), m_regex_check->isChecked());
	});

	connect(
	    m_replace_all_btn,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		emit replace_all_requested(
		    m_find_field->text(), m_replace_field->text(), m_case_check->isChecked(), m_regex_check->isChecked());
	});
}
