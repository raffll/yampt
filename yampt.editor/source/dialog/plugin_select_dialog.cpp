#include "plugin_select_dialog.hpp"
#include <filesystem>
#include <QHBoxLayout>

plugin_select_dialog_t::plugin_select_dialog_t(const std::vector<std::string> & available_files, QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle("Select Plugins");
	setModal(true);
	resize(400, 500);

	m_list = new QListWidget(this);
	for (const auto & path : available_files)
	{
		auto filename = std::filesystem::path(path).filename().string();
		auto * item = new QListWidgetItem(QString::fromStdString(filename));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Checked);
		item->setData(Qt::UserRole, QString::fromStdString(path));
		m_list->addItem(item);
	}

	auto * btn_select_all = new QPushButton("Select All", this);
	auto * btn_select_none = new QPushButton("Select None", this);

	auto * select_layout = new QHBoxLayout;
	select_layout->addWidget(btn_select_all);
	select_layout->addWidget(btn_select_none);
	select_layout->addStretch();

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

	auto * main_layout = new QVBoxLayout(this);
	main_layout->addWidget(m_list);
	main_layout->addLayout(select_layout);
	main_layout->addWidget(m_buttons);

	connect(
	    btn_select_all,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		for (int i = 0; i < m_list->count(); ++i)
			m_list->item(i)->setCheckState(Qt::Checked);
	});

	connect(
	    btn_select_none,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		for (int i = 0; i < m_list->count(); ++i)
			m_list->item(i)->setCheckState(Qt::Unchecked);
	});

	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

std::vector<std::string> plugin_select_dialog_t::selected_paths() const
{
	std::vector<std::string> result;
	for (int i = 0; i < m_list->count(); ++i)
	{
		const auto * item = m_list->item(i);
		if (item->checkState() == Qt::Checked)
			result.push_back(item->data(Qt::UserRole).toString().toStdString());
	}
	return result;
}
