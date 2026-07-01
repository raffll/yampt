#include "dialog/shortcuts_settings_view.hpp"
#include <io/app_settings.hpp>

#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

shortcuts_settings_view_t::shortcuts_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
    entries_ = {
        {"copy_original", "Copy Original", "F8"},
        {"set_in_progress", "Set In Progress", "F9"},
        {"set_translated", "Set Translated", "F10"},
        {"set_untranslated", "Set Untranslated", "Del"},
    };

    auto * layout = new QVBoxLayout(this);

    table_ = new QTableWidget(static_cast<int>(entries_.size()), 2, this);
    table_->setHorizontalHeaderLabels({"Action", "Shortcut"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int row = 0; row < static_cast<int>(entries_.size()); ++row)
    {
        const auto & entry = entries_[static_cast<size_t>(row)];

        auto * action_item = new QTableWidgetItem(
            QString::fromStdString(entry.display_name));
        action_item->setFlags(action_item->flags() & ~Qt::ItemIsEditable);
        table_->setItem(row, 0, action_item);

        auto * shortcut_item = new QTableWidgetItem(
            QString::fromStdString(entry.default_sequence));
        shortcut_item->setFlags(shortcut_item->flags() & ~Qt::ItemIsEditable);
        table_->setItem(row, 1, shortcut_item);
    }

    layout->addWidget(table_);

    conflict_label_ = new QLabel(this);
    conflict_label_->setStyleSheet("color: red;");
    conflict_label_->setVisible(false);
    layout->addWidget(conflict_label_);

    reset_button_ = new QPushButton("Reset to Defaults", this);
    reset_button_->setToolTip("Restore all shortcuts to defaults");
    layout->addWidget(reset_button_);

    connect(table_, &QTableWidget::cellDoubleClicked,
            this, &shortcuts_settings_view_t::on_cell_double_clicked);

    connect(reset_button_, &QPushButton::clicked,
            this, &shortcuts_settings_view_t::reset_to_defaults);
}

void shortcuts_settings_view_t::load(const app_settings_t & settings)
{
    for (int row = 0; row < static_cast<int>(entries_.size()); ++row)
    {
        const auto & entry = entries_[static_cast<size_t>(row)];
        const auto sequence = settings.shortcut(entry.action_name);
        const auto & display = sequence.empty() ? entry.default_sequence : sequence;
        table_->item(row, 1)->setText(QString::fromStdString(display));
    }

    check_conflicts();
}

void shortcuts_settings_view_t::apply(app_settings_t & settings) const
{
    for (int row = 0; row < static_cast<int>(entries_.size()); ++row)
    {
        const auto & entry = entries_[static_cast<size_t>(row)];
        const auto sequence = table_->item(row, 1)->text().toStdString();
        settings.set_shortcut(entry.action_name, sequence);
    }
}

bool shortcuts_settings_view_t::has_conflicts() const
{
    return has_conflicts_;
}

void shortcuts_settings_view_t::on_cell_double_clicked(int row, int column)
{
    if (column != 1)
        return;

    auto * editor = new QKeySequenceEdit(table_);
    const auto current_text = table_->item(row, 1)->text();
    editor->setKeySequence(QKeySequence(current_text));
    table_->setCellWidget(row, 1, editor);
    editor->setFocus();

    connect(editor, &QKeySequenceEdit::editingFinished, this, [this, row, editor]() {
        const auto sequence = editor->keySequence().toString();
        table_->removeCellWidget(row, 1);
        table_->item(row, 1)->setText(sequence);
        check_conflicts();
    });
}

void shortcuts_settings_view_t::check_conflicts()
{
    const auto row_count = static_cast<int>(entries_.size());

    for (int row = 0; row < row_count; ++row)
        table_->item(row, 1)->setBackground(QBrush());

    bool found_conflict = false;
    QString conflict_message;

    for (int first = 0; first < row_count - 1; ++first)
    {
        const auto first_text = table_->item(first, 1)->text();
        if (first_text.isEmpty())
            continue;

        for (int second = first + 1; second < row_count; ++second)
        {
            const auto second_text = table_->item(second, 1)->text();
            if (first_text != second_text)
                continue;

            table_->item(first, 1)->setBackground(QColor(255, 200, 200));
            table_->item(second, 1)->setBackground(QColor(255, 200, 200));
            found_conflict = true;

            const auto & conflicting_name = entries_[static_cast<size_t>(second)].display_name;
            conflict_message = QString("Conflict with: %1")
                .arg(QString::fromStdString(conflicting_name));
        }
    }

    conflict_label_->setText(conflict_message);
    conflict_label_->setVisible(found_conflict);

    if (has_conflicts_ != found_conflict)
    {
        has_conflicts_ = found_conflict;
        emit conflict_state_changed(has_conflicts_);
    }
}

void shortcuts_settings_view_t::reset_to_defaults()
{
    for (int row = 0; row < static_cast<int>(entries_.size()); ++row)
    {
        const auto & entry = entries_[static_cast<size_t>(row)];
        table_->item(row, 1)->setText(QString::fromStdString(entry.default_sequence));
    }

    check_conflicts();
}
