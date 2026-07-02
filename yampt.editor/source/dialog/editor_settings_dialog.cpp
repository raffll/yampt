#include "dialog/editor_settings_dialog.hpp"
#include "dialog/appearance_settings_view.hpp"
#include "dialog/editor_paths_view.hpp"
#include <io/app_settings.hpp>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

editor_settings_dialog_t::editor_settings_dialog_t(app_settings_t & settings, QWidget * parent)
    : QDialog(parent)
    , m_settings(settings)
{
	setWindowTitle("Settings");
	setMinimumSize(600, 450);

	m_category_list = new QListWidget(this);
	m_category_list->setFixedWidth(150);

	m_content_stack = new QStackedWidget(this);

	m_paths_view = new editor_paths_view_t(this);
	m_appearance_view = new appearance_settings_view_t(this);

	m_category_list->addItem("Paths");
	m_category_list->addItem("Appearance");

	m_content_stack->addWidget(m_paths_view);
	m_content_stack->addWidget(m_appearance_view);

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
	m_category_list->setCurrentRow(0);
}

void editor_settings_dialog_t::apply_all()
{
	m_paths_view->apply(m_settings);
	m_appearance_view->save(m_settings);
	m_settings.sync();
}
