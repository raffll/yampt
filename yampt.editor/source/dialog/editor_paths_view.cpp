#include "dialog/editor_paths_view.hpp"
#include <settings_store.hpp>
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

void editor_paths_view_t::load(const settings_store_t &)
{}

void editor_paths_view_t::apply(settings_store_t &) const
{}
