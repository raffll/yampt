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
    m_editable_entries = {
        {"copy_original", "Copy Original", "F8", true},
        {"set_in_progress", "Set In Progress", "F9", true},
        {"set_translated", "Set Translated", "F10", true},
        {"save", "Save", "Ctrl+S", true},
        {"find", "Find / Replace", "Ctrl+F", true},
        {"settings", "Open Settings", "Ctrl+,", true},
        {"escape", "Escape", "Escape", true},
    };

    m_readonly_entries = {
        {"set_untranslated", "Set Untranslated (table focus)", "Del", false},
        {"navigate_next", "Next Entry", "Shift+Return", false},
        {"navigate_prev", "Previous Entry", "Ctrl+Up", false},
    };

    m_total_row_count = static_cast<int>(m_editable_entries.size() + m_readonly_entries.size());

    auto * layout = new QVBoxLayout(this);

    auto * editable_label = new QLabel("Application Shortcuts (double-click to edit):", this);
    layout->addWidget(editable_label);

    m_table = new QTableWidget(m_total_row_count, 2, this);
    m_table->setHorizontalHeaderLabels({"Action", "Shortcut"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    int row = 0;

    for (const auto & entry : m_editable_entries)
    {
        auto * action_item = new QTableWidgetItem(
            QString::fromStdString(entry.display_name));
        action_item->setFlags(action_item->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 0, action_item);

        auto * shortcut_item = new QTableWidgetItem(
            QString::fromStdString(entry.default_sequence));
        shortcut_item->setFlags(shortcut_item->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 1, shortcut_item);

        ++row;
    }

    for (const auto & entry : m_readonly_entries)
    {
        auto * action_item = new QTableWidgetItem(
            QString::fromStdString(entry.display_name));
        action_item->setFlags(action_item->flags() & ~Qt::ItemIsEditable);
        action_item->setForeground(QColor(128, 128, 128));
        m_table->setItem(row, 0, action_item);

        auto * shortcut_item = new QTableWidgetItem(
            QString::fromStdString(entry.default_sequence));
        shortcut_item->setFlags(shortcut_item->flags() & ~Qt::ItemIsEditable);
        shortcut_item->setForeground(QColor(128, 128, 128));
        m_table->setItem(row, 1, shortcut_item);

        ++row;
    }

    layout->addWidget(m_table);

    m_conflict_label = new QLabel(this);
    m_conflict_label->setStyleSheet("color: red;");
    m_conflict_label->setVisible(false);
    layout->addWidget(m_conflict_label);

    m_reset_button = new QPushButton("Reset to Defaults", this);
    m_reset_button->setToolTip("Restore all shortcuts to defaults");
    layout->addWidget(m_reset_button);

    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &shortcuts_settings_view_t::on_cell_double_clicked);

    connect(m_reset_button, &QPushButton::clicked,
            this, &shortcuts_settings_view_t::reset_to_defaults);
}

void shortcuts_settings_view_t::load(const app_settings_t & settings)
{
    const auto editable_count = static_cast<int>(m_editable_entries.size());

    for (int row = 0; row < editable_count; ++row)
    {
        const auto & entry = m_editable_entries[static_cast<size_t>(row)];
        const auto sequence = settings.shortcut(entry.action_name);
        const auto & display = sequence.empty() ? entry.default_sequence : sequence;
        m_table->item(row, 1)->setText(QString::fromStdString(display));
    }

    check_conflicts();
}

void shortcuts_settings_view_t::apply(app_settings_t & settings) const
{
    const auto editable_count = static_cast<int>(m_editable_entries.size());

    for (int row = 0; row < editable_count; ++row)
    {
        const auto & entry = m_editable_entries[static_cast<size_t>(row)];
        const auto sequence = m_table->item(row, 1)->text().toStdString();
        settings.set_shortcut(entry.action_name, sequence);
    }
}

bool shortcuts_settings_view_t::has_conflicts() const
{
    return m_has_conflicts;
}

void shortcuts_settings_view_t::on_cell_double_clicked(int row, int column)
{
    if (column != 1)
        return;

    const auto editable_count = static_cast<int>(m_editable_entries.size());
    if (row >= editable_count)
        return;

    auto * editor = new QKeySequenceEdit(m_table);
    const auto current_text = m_table->item(row, 1)->text();
    editor->setKeySequence(QKeySequence(current_text));
    m_table->setCellWidget(row, 1, editor);
    editor->setFocus();

    connect(editor, &QKeySequenceEdit::editingFinished, this, [this, row, editor]() {
        const auto sequence = editor->keySequence().toString();
        m_table->removeCellWidget(row, 1);
        m_table->item(row, 1)->setText(sequence);
        check_conflicts();
    });
}

void shortcuts_settings_view_t::check_conflicts()
{
    const auto editable_count = static_cast<int>(m_editable_entries.size());

    for (int row = 0; row < editable_count; ++row)
        m_table->item(row, 1)->setBackground(QBrush());

    bool found_conflict = false;
    QString conflict_message;

    for (int first = 0; first < editable_count - 1; ++first)
    {
        const auto first_text = m_table->item(first, 1)->text();
        if (first_text.isEmpty())
            continue;

        for (int second = first + 1; second < editable_count; ++second)
        {
            const auto second_text = m_table->item(second, 1)->text();
            if (first_text != second_text)
                continue;

            m_table->item(first, 1)->setBackground(QColor(255, 200, 200));
            m_table->item(second, 1)->setBackground(QColor(255, 200, 200));
            found_conflict = true;

            const auto & conflicting_name = m_editable_entries[static_cast<size_t>(second)].display_name;
            conflict_message = QString("Conflict: \"%1\" and \"%2\" share the same shortcut")
                .arg(QString::fromStdString(m_editable_entries[static_cast<size_t>(first)].display_name))
                .arg(QString::fromStdString(conflicting_name));
        }
    }

    m_conflict_label->setText(conflict_message);
    m_conflict_label->setVisible(found_conflict);

    if (m_has_conflicts != found_conflict)
    {
        m_has_conflicts = found_conflict;
        emit conflict_state_changed(m_has_conflicts);
    }
}

void shortcuts_settings_view_t::reset_to_defaults()
{
    const auto editable_count = static_cast<int>(m_editable_entries.size());

    for (int row = 0; row < editable_count; ++row)
    {
        const auto & entry = m_editable_entries[static_cast<size_t>(row)];
        m_table->item(row, 1)->setText(QString::fromStdString(entry.default_sequence));
    }

    check_conflicts();
}
