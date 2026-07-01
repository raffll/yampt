#include "dialog/editor_settings_dialog.hpp"
#include "dialog/editor_paths_view.hpp"
#include <io/app_settings.hpp>

#include <QDialogButtonBox>
#include <QVBoxLayout>

editor_settings_dialog_t::editor_settings_dialog_t(app_settings_t & settings, QWidget * parent)
    : QDialog(parent)
    , settings_(settings)
{
    setWindowTitle("Settings");
    setMinimumSize(400, 200);

    paths_view_ = new editor_paths_view_t(this);
    paths_view_->load(settings_);

    button_box_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto * layout = new QVBoxLayout(this);
    layout->addWidget(paths_view_);
    layout->addWidget(button_box_);

    connect(button_box_, &QDialogButtonBox::accepted, this, [this]()
    {
        apply_all();
        accept();
    });

    connect(button_box_, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void editor_settings_dialog_t::apply_all()
{
    paths_view_->apply(settings_);
    settings_.sync();
}
