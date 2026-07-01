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

    m_path_list = new QListWidget(this);
    layout->addWidget(m_path_list, 1);

    auto * button_column = new QVBoxLayout;

    m_add_button = new QPushButton("Add...", this);
    m_add_button->setToolTip("Add a workspace directory");
    button_column->addWidget(m_add_button);

    m_remove_button = new QPushButton("Remove", this);
    m_remove_button->setToolTip("Remove selected directory");
    button_column->addWidget(m_remove_button);

    m_move_up_button = new QPushButton("Move Up", this);
    m_move_up_button->setToolTip("Move selected directory up in priority");
    button_column->addWidget(m_move_up_button);

    m_move_down_button = new QPushButton("Move Down", this);
    m_move_down_button->setToolTip("Move selected directory down in priority");
    button_column->addWidget(m_move_down_button);

    button_column->addStretch();
    layout->addLayout(button_column);

    connect(m_add_button, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_add_clicked);

    connect(m_remove_button, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_remove_clicked);

    connect(m_move_up_button, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_move_up_clicked);

    connect(m_move_down_button, &QPushButton::clicked,
            this, &workspace_settings_view_t::on_move_down_clicked);

    connect(m_path_list, &QListWidget::currentRowChanged,
            this, [this](int) { update_button_states(); });

    update_button_states();
}

void workspace_settings_view_t::load(const app_settings_t & settings)
{
    m_path_list->clear();

    const auto roots = settings.workspace_roots();
    for (const auto & root : roots)
        m_path_list->addItem(QString::fromStdString(root));

    update_button_states();
}

void workspace_settings_view_t::apply(app_settings_t & settings) const
{
    std::vector<std::string> roots;
    roots.reserve(static_cast<size_t>(m_path_list->count()));

    for (int index = 0; index < m_path_list->count(); ++index)
        roots.push_back(m_path_list->item(index)->text().toStdString());

    settings.set_workspace_roots(roots);
}

void workspace_settings_view_t::on_add_clicked()
{
    const auto directory = QFileDialog::getExistingDirectory(
        this, "Select Workspace Directory");

    if (directory.isEmpty())
        return;

    m_path_list->addItem(directory);
    m_path_list->setCurrentRow(m_path_list->count() - 1);
    update_button_states();
}

void workspace_settings_view_t::on_remove_clicked()
{
    const int current_row = m_path_list->currentRow();
    if (current_row < 0)
        return;

    delete m_path_list->takeItem(current_row);
    update_button_states();
}

void workspace_settings_view_t::on_move_up_clicked()
{
    const int current_row = m_path_list->currentRow();
    if (current_row <= 0)
        return;

    auto * item = m_path_list->takeItem(current_row);
    m_path_list->insertItem(current_row - 1, item);
    m_path_list->setCurrentRow(current_row - 1);
    update_button_states();
}

void workspace_settings_view_t::on_move_down_clicked()
{
    const int current_row = m_path_list->currentRow();
    if (current_row < 0 || current_row >= m_path_list->count() - 1)
        return;

    auto * item = m_path_list->takeItem(current_row);
    m_path_list->insertItem(current_row + 1, item);
    m_path_list->setCurrentRow(current_row + 1);
    update_button_states();
}

void workspace_settings_view_t::update_button_states()
{
    const int current_row = m_path_list->currentRow();
    const int total_count = m_path_list->count();
    const bool has_selection = (current_row >= 0);

    m_remove_button->setEnabled(has_selection);
    m_move_up_button->setEnabled(has_selection && current_row > 0);
    m_move_down_button->setEnabled(has_selection && current_row < total_count - 1);
}
