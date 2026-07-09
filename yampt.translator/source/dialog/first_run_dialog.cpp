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
	m_from_combo = new QComboBox(this);
	m_from_combo->addItem("English", QString("EN"));
	m_from_combo->addItem("Polish", QString("PL"));
	m_from_combo->addItem("German", QString("DE"));
	m_from_combo->addItem("French", QString("FR"));
	m_from_combo->addItem("Russian", QString("RU"));
	m_from_combo->addItem("Italian", QString("IT"));
	m_from_combo->addItem("Hungarian", QString("HU"));
	m_from_combo->setCurrentIndex(0);
	combo_layout->addWidget(m_from_combo);

	combo_layout->addWidget(new QLabel("To:"));
	m_to_combo = new QComboBox(this);
	m_to_combo->addItem("English", QString("EN"));
	m_to_combo->addItem("Polish", QString("PL"));
	m_to_combo->addItem("German", QString("DE"));
	m_to_combo->addItem("French", QString("FR"));
	m_to_combo->addItem("Russian", QString("RU"));
	m_to_combo->addItem("Italian", QString("IT"));
	m_to_combo->addItem("Hungarian", QString("HU"));
	m_to_combo->setCurrentIndex(1);
	combo_layout->addWidget(m_to_combo);

	layout->addLayout(combo_layout);

	auto * button_box = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	layout->addWidget(button_box);
}

std::string first_run_dialog_t::selected_foreign_language() const
{
	return m_from_combo->currentData().toString().toStdString();
}

std::string first_run_dialog_t::selected_native_language() const
{
	return m_to_combo->currentData().toString().toStdString();
}
