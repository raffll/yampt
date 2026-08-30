#include <resource_paths.hpp>
#include "first_run_dialog.hpp"
#include <utility/language_config.hpp>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

first_run_dialog_t::first_run_dialog_t(QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Language Setup"));
	setModal(true);

	auto * layout = new QVBoxLayout(this);

	auto * combo_layout = new QHBoxLayout;

	const auto languages =
	    language_config::load(resource_paths::languages_file());

	combo_layout->addWidget(new QLabel(tr("From:")));
	m_from_combo = new QComboBox(this);
	for (const auto & lang : languages)
		m_from_combo->addItem(QString::fromStdString(lang.display_name), QString::fromStdString(lang.code));
	m_from_combo->setCurrentIndex(0);
	combo_layout->addWidget(m_from_combo);

	combo_layout->addWidget(new QLabel(tr("To:")));
	m_to_combo = new QComboBox(this);
	for (const auto & lang : languages)
		m_to_combo->addItem(QString::fromStdString(lang.display_name), QString::fromStdString(lang.code));
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
