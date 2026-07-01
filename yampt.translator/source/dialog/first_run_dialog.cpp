#include "first_run_dialog.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

first_run_dialog_t::first_run_dialog_t(QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle("Language Setup");
	setModal(true);

	auto * layout = new QVBoxLayout(this);

	auto * combo_layout = new QHBoxLayout;

	combo_layout->addWidget(new QLabel("From:"));
	from_combo_ = new QComboBox(this);
	from_combo_->addItem("English", QString("EN"));
	from_combo_->addItem("Polish", QString("PL"));
	from_combo_->addItem("German", QString("DE"));
	from_combo_->addItem("French", QString("FR"));
	from_combo_->addItem("Russian", QString("RU"));
	from_combo_->setCurrentIndex(0);
	combo_layout->addWidget(from_combo_);

	combo_layout->addWidget(new QLabel("To:"));
	to_combo_ = new QComboBox(this);
	to_combo_->addItem("English", QString("EN"));
	to_combo_->addItem("Polish", QString("PL"));
	to_combo_->addItem("German", QString("DE"));
	to_combo_->addItem("French", QString("FR"));
	to_combo_->addItem("Russian", QString("RU"));
	to_combo_->setCurrentIndex(1);
	combo_layout->addWidget(to_combo_);

	layout->addLayout(combo_layout);

	auto * button_box = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	layout->addWidget(button_box);
}

std::string first_run_dialog_t::selected_foreign_language() const
{
	return from_combo_->currentData().toString().toStdString();
}

std::string first_run_dialog_t::selected_native_language() const
{
	return to_combo_->currentData().toString().toStdString();
}
