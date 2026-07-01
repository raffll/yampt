#include "dialog/workspace_settings_view.hpp"
#include <io/app_settings.hpp>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

workspace_settings_view_t::workspace_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
    auto * layout = new QHBoxLayout(this);

    path_list_ = new QListWidget(this);
    layout->addWidget(path_list_, 1);

    auto * button_column = new QVBoxLayout;

    add_button_ = new QPushButton("Add...", this);
    add_button_->setToolTip("Add a workspace directory");
    button_column->addWidget(add_button_);

    remove_button_ = new QPushButton("Remove", this);
    remove_button_->setToolTip("Remove selected directory");
    button_column->addWidget(remove_button_);

    move_up_button_ = new QPushButton("Move Up", this);
    move_up_button_->setToolTip("Move selected directory up in priority");
    button_column->addWidget(move_up_button_);

    move_down_button_ = new QPushButton("Move Down", this);
    move_down_button_->setToolTip("Move selected directory down in priority");
    button_column->addWidget(move_down_button_);

    button_column->addStretch();
    layout->addLayout(button_column);

    connect(add_button_, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_add_clicked);

    connect(remove_button_, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_remove_clicked);

    connect(move_up_button_, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_move_up_clicked);

    connect(move_down_button_, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_move_down_clicked);

    connect(path_list_, &QListWidget::currentRowChanged,
            this, [this](int) { update_button_states(); });

    update_button_states();
}

void workspace_settings_view_t::load(const app_settings_t & settings)
{
    path_list_->clear();

    const auto roots = settings.workspace_roots();
    for (const auto & root : roots)
        path_list_->addItem(QString::fromStdString(root));

    update_button_states();
}

void workspace_settings_view_t::apply(app_settings_t & settings) const
{
    std::vector<std::string> roots;
    roots.reserve(static_cast<size_t>(path_list_->count()));

    for (int index = 0; index < path_list_->count(); ++index)
        roots.push_back(path_list_->item(index)->text().toStdString());

    settings.set_workspace_roots(roots);
}

void workspace_settings_view_t::on_add_clicked()
{
    const auto directory = QFileDialog::getExistingDirectory(
        this, "Select Workspace Directory");

    if (directory.isEmpty())
        return;

    path_list_->addItem(directory);
    path_list_->setCurrentRow(path_list_->count() - 1);
    update_button_states();
}

void workspace_settings_view_t::on_remove_clicked()
{
    const int current_row = path_list_->currentRow();
    if (current_row < 0)
        return;

    delete path_list_->takeItem(current_row);
    update_button_states();
}

void workspace_settings_view_t::on_move_up_clicked()
{
    const int current_row = path_list_->currentRow();
    if (current_row <= 0)
        return;

    auto * item = path_list_->takeItem(current_row);
    path_list_->insertItem(current_row - 1, item);
    path_list_->setCurrentRow(current_row - 1);
    update_button_states();
}

void workspace_settings_view_t::on_move_down_clicked()
{
    const int current_row = path_list_->currentRow();
    if (current_row < 0 || current_row >= path_list_->count() - 1)
        return;

    auto * item = path_list_->takeItem(current_row);
    path_list_->insertItem(current_row + 1, item);
    path_list_->setCurrentRow(current_row + 1);
    update_button_states();
}

void workspace_settings_view_t::update_button_states()
{
    const int current_row = path_list_->currentRow();
    const int total_count = path_list_->count();
    const bool has_selection = (current_row >= 0);

    remove_button_->setEnabled(has_selection);
    move_up_button_->setEnabled(has_selection && current_row > 0);
    move_down_button_->setEnabled(has_selection && current_row < total_count - 1);
}
