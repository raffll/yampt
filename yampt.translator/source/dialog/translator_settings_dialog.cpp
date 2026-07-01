#include "dialog/translator_settings_dialog.hpp"
#include "dialog/language_settings_view.hpp"
#include "dialog/shortcuts_settings_view.hpp"
#include "dialog/translation_settings_view.hpp"
#include <io/app_settings.hpp>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

translator_settings_dialog_t::translator_settings_dialog_t(
    app_settings_t & settings,
    const std::string & dictionaries_dir,
    QWidget * parent)
    : QDialog(parent)
    , m_settings(settings)
{
	setWindowTitle("Settings");
	setMinimumSize(600, 450);

	m_category_list = new QListWidget(this);
	m_category_list->setFixedWidth(150);
	m_category_list->setFocusPolicy(Qt::NoFocus);

	m_content_stack = new QStackedWidget(this);

	m_language_view = new language_settings_view_t(dictionaries_dir, this);
	m_translation_view = new translation_settings_view_t(this);
	m_shortcuts_view = new shortcuts_settings_view_t(this);

	m_category_list->addItem("Language");
	m_category_list->addItem("Shortcuts");

	m_content_stack->addWidget(m_language_view);
	m_content_stack->addWidget(m_shortcuts_view);
	m_content_stack->addWidget(m_translation_view);

	connect(m_category_list, &QListWidget::currentRowChanged, m_content_stack, &QStackedWidget::setCurrentIndex);

	m_button_box =
	    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);

	m_apply_button = m_button_box->button(QDialogButtonBox::Apply);

	connect(
	    m_button_box,
	    &QDialogButtonBox::accepted,
	    this,
	    [this]()
	{
		apply_all();
		accept();
	});

	connect(
	    m_apply_button,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		apply_all();
		emit settings_applied("all");
	});

	connect(m_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(
	    m_shortcuts_view,
	    &shortcuts_settings_view_t::conflict_state_changed,
	    this,
	    &translator_settings_dialog_t::update_ok_button_state);

	auto * content_layout = new QHBoxLayout;
	content_layout->addWidget(m_category_list);
	content_layout->addWidget(m_content_stack, 1);

	auto * main_layout = new QVBoxLayout(this);
	main_layout->addLayout(content_layout, 1);
	main_layout->addWidget(m_button_box);

	m_language_view->load(m_settings);
	m_translation_view->load(m_settings);
	m_shortcuts_view->load(m_settings);
	m_category_list->setCurrentRow(0);
}

void translator_settings_dialog_t::apply_all()
{
	m_language_view->apply(m_settings);
	m_translation_view->apply(m_settings);
	m_shortcuts_view->apply(m_settings);
	m_settings.sync();
}

void translator_settings_dialog_t::update_ok_button_state()
{
	const bool has_conflicts = m_shortcuts_view->has_conflicts();
	m_button_box->button(QDialogButtonBox::Ok)->setEnabled(!has_conflicts);
	m_apply_button->setEnabled(!has_conflicts);
}
