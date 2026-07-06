#include "editor_paths_view.hpp"
#include <settings_store.hpp>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

editor_paths_view_t::editor_paths_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);

	auto * header = new QLabel("Merged patch output path (relative to base directory):", this);
	layout->addWidget(header);

	layout->addSpacing(8);

	auto * folder_label = new QLabel("Folder mode:", this);
	layout->addWidget(folder_label);
	m_edt_folder_path = new QLineEdit(this);
	m_edt_folder_path->setPlaceholderText("Merged Patch.esp");
	layout->addWidget(m_edt_folder_path);

	layout->addSpacing(8);

	auto * mo2_label = new QLabel("MO2 mode:", this);
	layout->addWidget(mo2_label);
	m_edt_mo2_path = new QLineEdit(this);
	m_edt_mo2_path->setPlaceholderText("../../overwrite/Merged Patch.esp");
	layout->addWidget(m_edt_mo2_path);

	layout->addSpacing(8);

	auto * openmw_label = new QLabel("OpenMW mode:", this);
	layout->addWidget(openmw_label);
	m_edt_openmw_path = new QLineEdit(this);
	m_edt_openmw_path->setPlaceholderText("data/Merged Patch.esp");
	layout->addWidget(m_edt_openmw_path);

	layout->addStretch();
}

void editor_paths_view_t::load(const settings_store_t & settings)
{
	m_edt_folder_path->setText(QString::fromStdString(settings.merge_path_folder()));
	m_edt_mo2_path->setText(QString::fromStdString(settings.merge_path_mo2()));
	m_edt_openmw_path->setText(QString::fromStdString(settings.merge_path_openmw()));
}

void editor_paths_view_t::apply(settings_store_t & settings) const
{
	settings.set_merge_path_folder(m_edt_folder_path->text().toStdString());
	settings.set_merge_path_mo2(m_edt_mo2_path->text().toStdString());
	settings.set_merge_path_openmw(m_edt_openmw_path->text().toStdString());
}
