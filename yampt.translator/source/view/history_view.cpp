#include "history_view.hpp"
#include "../editor/edit_history.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

history_view_t::history_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	auto * scroll_area = new QScrollArea(this);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);

	m_scroll_content = new QWidget(scroll_area);
	m_entries_layout = new QVBoxLayout(m_scroll_content);
	m_entries_layout->setContentsMargins(4, 4, 4, 4);
	m_entries_layout->setSpacing(4);
	m_entries_layout->addStretch();

	scroll_area->setWidget(m_scroll_content);
	layout->addWidget(scroll_area);
}

void history_view_t::update_history(const std::vector<history_entry_t> & entries, bool allow_revert)
{
	clear();

	for (size_t i = entries.size(); i > 0; --i)
	{
		const auto & entry = entries[i - 1];
		size_t history_index = i - 1;

		auto * row = new QWidget(m_scroll_content);
		auto * row_layout = new QVBoxLayout(row);
		row_layout->setContentsMargins(2, 2, 2, 2);
		row_layout->setSpacing(2);

		auto timestamp_str = QString::fromStdString(entry.timestamp);
		auto value_str = QString::fromStdString(entry.value);
		if (value_str.length() > 80)
			value_str = value_str.left(80) + "...";

		auto * label = new QLabel(QString("[%1] %2").arg(timestamp_str, value_str), row);
		label->setWordWrap(true);
		row_layout->addWidget(label);

		if (allow_revert)
		{
			auto * button_layout = new QHBoxLayout();
			button_layout->setContentsMargins(0, 0, 0, 0);

			auto * revert_btn = new QPushButton(tr("Revert"), row);
			connect(
			    revert_btn,
			    &QPushButton::clicked,
			    this,
			    [this, history_index]() { emit revert_requested(history_index); });

			button_layout->addWidget(revert_btn);
			button_layout->addStretch();
			row_layout->addLayout(button_layout);
		}

		m_entries_layout->insertWidget(m_entries_layout->count() - 1, row);
	}
}

void history_view_t::clear()
{
	while (m_entries_layout->count() > 1)
	{
		auto * item = m_entries_layout->takeAt(0);
		if (item->widget())
		{
			delete item->widget();
		}
		delete item;
	}
}
