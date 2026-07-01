#include "dialog/translator_settings_dialog.hpp"
#include "dialog/language_settings_view.hpp"
#include "dialog/translation_settings_view.hpp"
#include "dialog/shortcuts_settings_view.hpp"
#include "dialog/workspace_settings_view.hpp"
#include <io/app_settings.hpp>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

translator_settings_dialog_t::translator_settings_dialog_t(app_settings_t & settings,
                                                            const std::string & dictionaries_dir,
                                                            QWidget * parent)
    : QDialog(parent)
    , settings_(settings)
{
    setWindowTitle("Settings");
    setMinimumSize(600, 450);

    category_list_ = new QListWidget(this);
    category_list_->setFixedWidth(150);

    content_stack_ = new QStackedWidget(this);

    language_view_ = new language_settings_view_t(dictionaries_dir, this);
    translation_view_ = new translation_settings_view_t(this);
    shortcuts_view_ = new shortcuts_settings_view_t(this);
    workspace_view_ = new workspace_settings_view_t(this);

    category_list_->addItem("Language");
    category_list_->addItem("Translation");
    category_list_->addItem("Shortcuts");
    category_list_->addItem("Workspace");

    content_stack_->addWidget(language_view_);
    content_stack_->addWidget(translation_view_);
    content_stack_->addWidget(shortcuts_view_);
    content_stack_->addWidget(workspace_view_);

    connect(category_list_, &QListWidget::currentRowChanged,
            content_stack_, &QStackedWidget::setCurrentIndex);

    button_box_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);

    apply_button_ = button_box_->button(QDialogButtonBox::Apply);

    connect(button_box_, &QDialogButtonBox::accepted, this, [this]()
    {
        apply_all();
        accept();
    });

    connect(apply_button_, &QPushButton::clicked, this, [this]()
    {
        apply_all();
        emit settings_applied("all");
    });

    connect(button_box_, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(shortcuts_view_, &shortcuts_settings_view_t::conflict_state_changed,
            this, &translator_settings_dialog_t::update_ok_button_state);

    auto * content_layout = new QHBoxLayout;
    content_layout->addWidget(category_list_);
    content_layout->addWidget(content_stack_, 1);

    auto * main_layout = new QVBoxLayout(this);
    main_layout->addLayout(content_layout, 1);
    main_layout->addWidget(button_box_);

    language_view_->load(settings_);
    translation_view_->load(settings_);
    shortcuts_view_->load(settings_);
    workspace_view_->load(settings_);
    category_list_->setCurrentRow(0);
}

void translator_settings_dialog_t::apply_all()
{
    language_view_->apply(settings_);
    translation_view_->apply(settings_);
    shortcuts_view_->apply(settings_);
    workspace_view_->apply(settings_);
    settings_.sync();
}

void translator_settings_dialog_t::update_ok_button_state()
{
    const bool has_conflicts = shortcuts_view_->has_conflicts();
    button_box_->button(QDialogButtonBox::Ok)->setEnabled(!has_conflicts);
    apply_button_->setEnabled(!has_conflicts);
}
