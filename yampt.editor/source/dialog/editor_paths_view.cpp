#include "dialog/editor_paths_view.hpp"
#include <io/app_settings.hpp>
#include <QFormLayout>
#include <QLineEdit>

editor_paths_view_t::editor_paths_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QFormLayout(this);

	m_openmw_merge_path_edit = new QLineEdit(this);
	m_openmw_merge_path_edit->setToolTip("Relative path from openmw.cfg directory to merge output folder");

	m_mo2_merge_path_edit = new QLineEdit(this);
	m_mo2_merge_path_edit->setToolTip("Relative path from MO2 profile directory to merge output folder");

	m_merge_filename_edit = new QLineEdit(this);
	m_merge_filename_edit->setToolTip("Filename of the merged patch");

	layout->addRow("OpenMW Merge Path:", m_openmw_merge_path_edit);
	layout->addRow("MO2 Merge Path:", m_mo2_merge_path_edit);
	layout->addRow("Merge Filename:", m_merge_filename_edit);
}

void editor_paths_view_t::load(const app_settings_t & settings)
{
	m_openmw_merge_path_edit->setText(QString::fromStdString(settings.openmw_merge_path()));
	m_mo2_merge_path_edit->setText(QString::fromStdString(settings.mo2_merge_path()));
	m_merge_filename_edit->setText(QString::fromStdString(settings.merge_output_path()));
}

void editor_paths_view_t::apply(app_settings_t & settings) const
{
	settings.set_openmw_merge_path(m_openmw_merge_path_edit->text().toStdString());
	settings.set_mo2_merge_path(m_mo2_merge_path_edit->text().toStdString());
	settings.set_merge_output_path(m_merge_filename_edit->text().toStdString());
}
