#include "editor_settings_dialog.hpp"
#include "appearance_settings_view.hpp"
#include "cleaning_settings_view.hpp"
#include "editor_paths_view.hpp"
#include "merge_settings_view.hpp"
#include "sub_record_rules_view.hpp"
#include <settings_store.hpp>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

editor_settings_dialog_t::editor_settings_dialog_t(settings_store_t & settings, QWidget * parent)
    : QDialog(parent)
    , m_settings(settings)
{
	setWindowTitle(tr("Settings"));
	setMinimumSize(600, 450);

	m_category_list = new QListWidget(this);
	m_category_list->setFixedWidth(150);

	m_content_stack = new QStackedWidget(this);

	m_paths_view = new editor_paths_view_t(this);
	m_appearance_view = new editor_appearance_settings_view_t(this);
	m_merge_view = new merge_settings_view_t(this);
	m_cleaning_view = new cleaning_settings_view_t(this);
	m_sub_record_rules_view = new sub_record_rules_view_t(this);

	m_category_list->addItem(tr("Appearance"));
	m_category_list->addItem(tr("Paths"));
	m_category_list->addItem(tr("Merged Patch"));
	m_category_list->addItem(tr("Cleaning"));
	m_category_list->addItem(tr("Sub-Record Rules"));

	m_content_stack->addWidget(m_appearance_view);
	m_content_stack->addWidget(m_paths_view);
	m_content_stack->addWidget(m_merge_view);
	m_content_stack->addWidget(m_cleaning_view);
	m_content_stack->addWidget(m_sub_record_rules_view);

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
		emit settings_applied("Paths");
	});

	connect(m_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto * content_layout = new QHBoxLayout;
	content_layout->addWidget(m_category_list);
	content_layout->addWidget(m_content_stack, 1);

	auto * main_layout = new QVBoxLayout(this);
	main_layout->addLayout(content_layout, 1);
	main_layout->addWidget(m_button_box);

	m_paths_view->load(m_settings);
	m_appearance_view->load(m_settings);
	m_merge_view->load(m_settings);
	m_cleaning_view->load(m_settings);
	m_sub_record_rules_view->load(m_settings);
	m_category_list->setCurrentRow(0);
}

void editor_settings_dialog_t::apply_all()
{
	m_paths_view->apply(m_settings);
	m_appearance_view->save(m_settings);
	m_merge_view->save(m_settings);
	m_cleaning_view->save(m_settings);
	m_sub_record_rules_view->save(m_settings);
	m_settings.sync();
}
