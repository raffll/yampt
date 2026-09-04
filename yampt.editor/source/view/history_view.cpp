#include "history_view.hpp"
#include <QListWidget>
#include <QVBoxLayout>

history_view_t::history_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	m_list = new QListWidget(this);
	layout->addWidget(m_list, 1);
}

void history_view_t::update_history(const std::vector<edit_log_entry_t> & entries)
{
	m_list->clear();

	for (size_t index = entries.size(); index > 0; --index)
	{
		const auto & entry = entries[index - 1];

		const auto text = QString("[%1] %2 — %3")
		                      .arg(
		                          QString::fromStdString(entry.timestamp),
		                          QString::fromStdString(entry.plugin_filename),
		                          QString::fromStdString(entry.description));

		new QListWidgetItem(text, m_list);
	}

	if (m_list->count() > 0)
		m_list->setCurrentRow(0);
}

void history_view_t::clear()
{
	m_list->clear();
}
