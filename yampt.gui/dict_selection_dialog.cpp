#include "dict_selection_dialog.hpp"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace
{

int canonical_order(const std::string & name)
{
    if (name.find("Morrowind") != std::string::npos)
        return 0;

    if (name.find("Tribunal") != std::string::npos)
        return 1;

    if (name.find("Bloodmoon") != std::string::npos)
        return 2;

    return 3;
}

std::vector<dict_selection_dialog_t::dict_entry_t> sort_entries(
    const std::vector<dict_selection_dialog_t::dict_entry_t> & entries)
{
    std::vector<dict_selection_dialog_t::dict_entry_t> base_entries;
    std::vector<dict_selection_dialog_t::dict_entry_t> user_entries;

    for (const auto & e : entries)
    {
        if (e.kind == dict_kind_t::base)
            base_entries.push_back(e);
        else
            user_entries.push_back(e);
    }

    std::stable_sort(base_entries.begin(), base_entries.end(),
        [](const dict_selection_dialog_t::dict_entry_t & a,
            const dict_selection_dialog_t::dict_entry_t & b)
        {
            return canonical_order(a.name) < canonical_order(b.name);
        });

    std::vector<dict_selection_dialog_t::dict_entry_t> result;
    result.reserve(base_entries.size() + user_entries.size());
    result.insert(result.end(), base_entries.begin(), base_entries.end());
    result.insert(result.end(), user_entries.begin(), user_entries.end());
    return result;
}

} // namespace

dict_selection_dialog_t::dict_selection_dialog_t(
    const std::vector<dict_entry_t> & entries, QWidget * parent)
    : QDialog(parent)
{
    setWindowTitle("Select Dictionaries");
    setModal(true);

    const auto sorted = sort_entries(entries);

    list_ = new QListWidget(this);
    for (const auto & entry : sorted)
    {
        const auto & kind_label = (entry.kind == dict_kind_t::base) ? "[BASE]" : "[USER]";
        auto display = QString::fromStdString(entry.name) + " " + kind_label;

        auto * item = new QListWidgetItem(display);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(entry.checked ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, QString::fromStdString(entry.path));
        list_->addItem(item);
    }

    up_button_ = new QPushButton("Up", this);
    down_button_ = new QPushButton("Down", this);

    auto * button_layout = new QVBoxLayout;
    button_layout->addWidget(up_button_);
    button_layout->addWidget(down_button_);
    button_layout->addStretch();

    auto * center_layout = new QHBoxLayout;
    center_layout->addWidget(list_);
    center_layout->addLayout(button_layout);

    button_box_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto * main_layout = new QVBoxLayout(this);
    main_layout->addLayout(center_layout);
    main_layout->addWidget(button_box_);

    connect(up_button_, &QPushButton::clicked, this, &dict_selection_dialog_t::on_move_up);
    connect(down_button_, &QPushButton::clicked, this, &dict_selection_dialog_t::on_move_down);
    connect(list_, &QListWidget::itemChanged, this, &dict_selection_dialog_t::on_check_changed);
    connect(button_box_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box_, &QDialogButtonBox::rejected, this, &QDialog::reject);

    update_ok_button();
}

std::vector<std::string> dict_selection_dialog_t::get_selected_paths() const
{
    std::vector<std::string> result;
    for (int i = 0; i < list_->count(); ++i)
    {
        const auto * item = list_->item(i);
        if (item->checkState() == Qt::Checked)
            result.push_back(item->data(Qt::UserRole).toString().toStdString());
    }
    return result;
}

void dict_selection_dialog_t::on_move_up()
{
    int row = list_->currentRow();
    if (row <= 0)
        return;

    auto * item = list_->takeItem(row);
    list_->insertItem(row - 1, item);
    list_->setCurrentRow(row - 1);
}

void dict_selection_dialog_t::on_move_down()
{
    int row = list_->currentRow();
    if (row < 0 || row >= list_->count() - 1)
        return;

    auto * item = list_->takeItem(row);
    list_->insertItem(row + 1, item);
    list_->setCurrentRow(row + 1);
}

void dict_selection_dialog_t::on_check_changed()
{
    update_ok_button();
}

void dict_selection_dialog_t::update_ok_button()
{
    bool any_checked = false;
    for (int i = 0; i < list_->count(); ++i)
    {
        if (list_->item(i)->checkState() == Qt::Checked)
        {
            any_checked = true;
            break;
        }
    }
    button_box_->button(QDialogButtonBox::Ok)->setEnabled(any_checked);
}
