#include "history_view.hpp"
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

history_view_t::history_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
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

void history_view_t::update_history(const std::vector<edit_log_entry_t> & entries)
{
	clear();

	for (size_t index = entries.size(); index > 0; --index)
	{
		const auto & entry = entries[index - 1];

		const auto text = QString("[%1] %2 — %3")
		                      .arg(
		                          QString::fromStdString(entry.timestamp),
		                          QString::fromStdString(entry.plugin_filename),
		                          QString::fromStdString(entry.description));

		auto * label = new QLabel(text, m_scroll_content);
		label->setWordWrap(true);
		m_entries_layout->insertWidget(m_entries_layout->count() - 1, label);
	}
}

void history_view_t::clear()
{
	while (m_entries_layout->count() > 1)
	{
		auto * item = m_entries_layout->takeAt(0);
		if (item->widget())
			delete item->widget();

		delete item;
	}
}
