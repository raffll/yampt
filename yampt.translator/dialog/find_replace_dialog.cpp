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
	layout->addWidget(new QLabel("Find:", this), 0, 0);
	find_field_ = new QLineEdit(this);
	layout->addWidget(find_field_, 0, 1, 1, 3);

	layout->addWidget(new QLabel("Replace:", this), 1, 0);
	replace_field_ = new QLineEdit(this);
	layout->addWidget(replace_field_, 1, 1, 1, 3);

	case_check_ = new QCheckBox("Case sensitive", this);
	layout->addWidget(case_check_, 2, 0, 1, 2);

	regex_check_ = new QCheckBox("Regex", this);
	layout->addWidget(regex_check_, 2, 2, 1, 2);

	find_next_btn_ = new QPushButton("Find Next", this);
	find_next_btn_->setToolTip("Find next matching entry");
	layout->addWidget(find_next_btn_, 3, 1);

	replace_btn_ = new QPushButton("Replace", this);
	replace_btn_->setToolTip("Replace current match and find next");
	layout->addWidget(replace_btn_, 3, 2);

	replace_all_btn_ = new QPushButton("Replace All", this);
	replace_all_btn_->setToolTip("Replace in all entries regardless of filters");
	layout->addWidget(replace_all_btn_, 3, 3);

	note_label_ = new QLabel("Replace All affects all entries regardless of filters.", this);
	note_label_->setStyleSheet("color: #888; font-style: italic;");
	layout->addWidget(note_label_, 4, 0, 1, 4);
}

void find_replace_dialog_t::connect_signals()
{
	connect(
	    find_next_btn_,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{ emit find_next_requested(find_field_->text(), case_check_->isChecked(), regex_check_->isChecked()); });

	connect(
	    replace_btn_,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		emit replace_requested(
		    find_field_->text(), replace_field_->text(), case_check_->isChecked(), regex_check_->isChecked());
	});

	connect(
	    replace_all_btn_,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		emit replace_all_requested(
		    find_field_->text(), replace_field_->text(), case_check_->isChecked(), regex_check_->isChecked());
	});
}
