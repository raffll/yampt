#include "dialog/editor_paths_view.hpp"
#include <io/app_settings.hpp>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

editor_paths_view_t::editor_paths_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QFormLayout(this);

	auto * openmw_row = new QHBoxLayout;
	m_openmw_data_edit = new QLineEdit(this);
	m_openmw_browse_button = new QPushButton("...", this);
	m_openmw_browse_button->setFixedWidth(30);
	m_openmw_browse_button->setToolTip("Browse for OpenMW data directory");
	openmw_row->addWidget(m_openmw_data_edit);
	openmw_row->addWidget(m_openmw_browse_button);

	auto * mo2_row = new QHBoxLayout;
	m_mo2_profile_edit = new QLineEdit(this);
	m_mo2_browse_button = new QPushButton("...", this);
	m_mo2_browse_button->setFixedWidth(30);
	m_mo2_browse_button->setToolTip("Browse for MO2 profile directory");
	mo2_row->addWidget(m_mo2_profile_edit);
	mo2_row->addWidget(m_mo2_browse_button);

	layout->addRow("OpenMW Data Directory:", openmw_row);
	layout->addRow("MO2 Profile Directory:", mo2_row);

	connect(m_openmw_browse_button, &QPushButton::clicked, this, &editor_paths_view_t::on_browse_openmw);

	connect(m_mo2_browse_button, &QPushButton::clicked, this, &editor_paths_view_t::on_browse_mo2);
}

void editor_paths_view_t::on_browse_openmw()
{
	const auto dir =
	    QFileDialog::getExistingDirectory(this, "Select OpenMW Data Directory", m_openmw_data_edit->text());

	if (!dir.isEmpty())
		m_openmw_data_edit->setText(dir);
}

void editor_paths_view_t::on_browse_mo2()
{
	const auto dir =
	    QFileDialog::getExistingDirectory(this, "Select MO2 Profile Directory", m_mo2_profile_edit->text());

	if (!dir.isEmpty())
		m_mo2_profile_edit->setText(dir);
}

void editor_paths_view_t::load(const app_settings_t & settings)
{
	m_openmw_data_edit->setText(QString::fromStdString(settings.openmw_data_dir()));
	m_mo2_profile_edit->setText(QString::fromStdString(settings.mo2_profile_dir()));
}

void editor_paths_view_t::apply(app_settings_t & settings) const
{
	settings.set_openmw_data_dir(m_openmw_data_edit->text().toStdString());
	settings.set_mo2_profile_dir(m_mo2_profile_edit->text().toStdString());
}
