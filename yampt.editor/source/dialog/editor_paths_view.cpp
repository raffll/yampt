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
    openmw_data_edit_ = new QLineEdit(this);
    openmw_browse_button_ = new QPushButton("...", this);
    openmw_browse_button_->setFixedWidth(30);
    openmw_browse_button_->setToolTip("Browse for OpenMW data directory");
    openmw_row->addWidget(openmw_data_edit_);
    openmw_row->addWidget(openmw_browse_button_);

    auto * mo2_row = new QHBoxLayout;
    mo2_profile_edit_ = new QLineEdit(this);
    mo2_browse_button_ = new QPushButton("...", this);
    mo2_browse_button_->setFixedWidth(30);
    mo2_browse_button_->setToolTip("Browse for MO2 profile directory");
    mo2_row->addWidget(mo2_profile_edit_);
    mo2_row->addWidget(mo2_browse_button_);

    layout->addRow("OpenMW Data Directory:", openmw_row);
    layout->addRow("MO2 Profile Directory:", mo2_row);

    connect(openmw_browse_button_, &QPushButton::clicked,
            this, &editor_paths_view_t::on_browse_openmw);

    connect(mo2_browse_button_, &QPushButton::clicked,
            this, &editor_paths_view_t::on_browse_mo2);
}

void editor_paths_view_t::on_browse_openmw()
{
    const auto dir = QFileDialog::getExistingDirectory(
        this, "Select OpenMW Data Directory", openmw_data_edit_->text());

    if (!dir.isEmpty())
        openmw_data_edit_->setText(dir);
}

void editor_paths_view_t::on_browse_mo2()
{
    const auto dir = QFileDialog::getExistingDirectory(
        this, "Select MO2 Profile Directory", mo2_profile_edit_->text());

    if (!dir.isEmpty())
        mo2_profile_edit_->setText(dir);
}

void editor_paths_view_t::load(const app_settings_t & settings)
{
    openmw_data_edit_->setText(QString::fromStdString(settings.openmw_data_dir()));
    mo2_profile_edit_->setText(QString::fromStdString(settings.mo2_profile_dir()));
}

void editor_paths_view_t::apply(app_settings_t & settings) const
{
    settings.set_openmw_data_dir(openmw_data_edit_->text().toStdString());
    settings.set_mo2_profile_dir(mo2_profile_edit_->text().toStdString());
}
