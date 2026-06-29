#include "first_run_dialog.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

first_run_dialog_t::first_run_dialog_t(const std::vector<std::string> & spell_languages, QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle("Setup");
	setModal(true);

	auto * layout = new QVBoxLayout(this);

	layout->addWidget(new QLabel("Text Encoding:"));
	encoding_combo_ = new QComboBox(this);
	encoding_combo_->addItem("Windows-1250");
	encoding_combo_->addItem("Windows-1251");
	encoding_combo_->addItem("Windows-1252");
	layout->addWidget(encoding_combo_);

	layout->addWidget(new QLabel("Spell Check Language:"));
	spell_lang_combo_ = new QComboBox(this);
	spell_lang_combo_->addItem("None");
	for (const auto & lang : spell_languages)
		spell_lang_combo_->addItem(QString::fromStdString(lang));
	layout->addWidget(spell_lang_combo_);

	auto * button_box = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	layout->addWidget(button_box);
}

int first_run_dialog_t::selected_encoding_index() const
{
	return encoding_combo_->currentIndex();
}

int first_run_dialog_t::selected_spell_lang_index() const
{
	return spell_lang_combo_->currentIndex();
}
