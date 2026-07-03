#include "dialog/editor_paths_view.hpp"
#include <app_settings.hpp>
#include <QFormLayout>
#include <QLabel>

editor_paths_view_t::editor_paths_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QFormLayout(this);
	layout->addRow(new QLabel(
	    "Merge output is automatic:\n"
	    "  Folder: same directory\n"
	    "  MO2: overwrite folder\n"
	    "  OpenMW: cfg_dir/data",
	    this));
}

void editor_paths_view_t::load(const app_settings_t &)
{}

void editor_paths_view_t::apply(app_settings_t &) const
{}
