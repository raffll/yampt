#include "lua_filter_view.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

lua_filter_view_t::lua_filter_view_t(QWidget * parent)
	: QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	setup_count_labels();
	layout->addWidget(m_total_label);
	layout->addWidget(m_severity_label);

	setup_severity_buttons();
	auto * button_row = new QHBoxLayout;
	button_row->addWidget(m_blocking_button);
	button_row->addWidget(m_mutating_button);
	button_row->addWidget(m_overlapping_button);
	button_row->addStretch();
	layout->addLayout(button_row);

	setup_interface_list();
	layout->addWidget(m_interface_list, 1);
}

void lua_filter_view_t::set_conflicts(const std::vector<handler_conflict_t> & conflicts)
{
	update_counts(conflicts);
	populate_interface_list(conflicts);
}

std::set<conflict_severity_t> lua_filter_view_t::enabled_severities() const
{
	std::set<conflict_severity_t> result;

	if (m_blocking_button->isChecked())
		result.insert(conflict_severity_t::blocking);

	if (m_mutating_button->isChecked())
		result.insert(conflict_severity_t::mutating);

	if (m_overlapping_button->isChecked())
		result.insert(conflict_severity_t::overlapping);

	return result;
}

std::set<std::string> lua_filter_view_t::enabled_interfaces() const
{
	std::set<std::string> result;

	for (int index = 0; index < m_interface_list->count(); ++index)
	{
		auto * item = m_interface_list->item(index);
		if (item->checkState() == Qt::Checked)
			result.insert(item->text().toStdString());
	}

	return result;
}

void lua_filter_view_t::setup_severity_buttons()
{
	m_blocking_button = new QPushButton(tr("Blocking"), this);
	m_blocking_button->setCheckable(true);
	m_blocking_button->setChecked(true);
	m_blocking_button->setToolTip(tr("Show conflicts where a handler cancels the operation"));

	m_mutating_button = new QPushButton(tr("Mutating"), this);
	m_mutating_button->setCheckable(true);
	m_mutating_button->setChecked(true);
	m_mutating_button->setToolTip(tr("Show conflicts where a handler modifies shared state"));

	m_overlapping_button = new QPushButton(tr("Overlapping"), this);
	m_overlapping_button->setCheckable(true);
	m_overlapping_button->setChecked(true);
	m_overlapping_button->setToolTip(tr("Show conflicts where multiple handlers coexist"));

	connect(m_blocking_button, &QPushButton::toggled, this, &lua_filter_view_t::filters_changed);
	connect(m_mutating_button, &QPushButton::toggled, this, &lua_filter_view_t::filters_changed);
	connect(m_overlapping_button, &QPushButton::toggled, this, &lua_filter_view_t::filters_changed);
}

void lua_filter_view_t::setup_interface_list()
{
	m_interface_list = new QListWidget(this);

	connect(m_interface_list, &QListWidget::itemChanged, this, &lua_filter_view_t::filters_changed);
}

void lua_filter_view_t::setup_count_labels()
{
	m_total_label = new QLabel(tr("Total: 0"), this);
	m_severity_label = new QLabel(tr("Blocking: 0 | Mutating: 0 | Overlapping: 0"), this);
}

void lua_filter_view_t::populate_interface_list(const std::vector<handler_conflict_t> & conflicts)
{
	m_interface_list->blockSignals(true);
	m_interface_list->clear();

	std::set<std::string> interface_names;
	for (const auto & conflict : conflicts)
		interface_names.insert(conflict.interface_name);

	for (const auto & name : interface_names)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(name), m_interface_list);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Checked);
	}

	m_interface_list->blockSignals(false);
}

void lua_filter_view_t::update_counts(const std::vector<handler_conflict_t> & conflicts)
{
	int blocking_count = 0;
	int mutating_count = 0;
	int overlapping_count = 0;

	for (const auto & conflict : conflicts)
	{
		switch (conflict.severity)
		{
		case conflict_severity_t::blocking: ++blocking_count; break;
		case conflict_severity_t::mutating: ++mutating_count; break;
		case conflict_severity_t::overlapping: ++overlapping_count; break;
		}
	}

	const int total = static_cast<int>(conflicts.size());
	m_total_label->setText(tr("Total: %1").arg(total));
	m_severity_label->setText(
		tr("Blocking: %1 | Mutating: %2 | Overlapping: %3")
			.arg(blocking_count)
			.arg(mutating_count)
			.arg(overlapping_count));
}
