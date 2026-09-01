#include "history_view.hpp"
#include "../editor/edit_history.hpp"
#include "status_display.hpp"
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>

namespace {

constexpr int history_index_role = Qt::UserRole + 1;
constexpr int max_value_display_length = 80;

} // namespace

history_view_t::history_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	m_list = new QListWidget(this);
	m_list->setContextMenuPolicy(Qt::CustomContextMenu);
	layout->addWidget(m_list, 1);

	connect(m_list, &QListWidget::customContextMenuRequested, this, &history_view_t::on_context_menu);
}

void history_view_t::update_history(const std::vector<history_entry_t> & entries, bool allow_revert)
{
	m_list->clear();
	m_allow_revert = allow_revert;

	for (size_t i = entries.size(); i > 0; --i)
	{
		const auto & entry = entries[i - 1];
		const size_t history_index = i - 1;

		auto timestamp_str = QString::fromStdString(entry.timestamp);
		auto status_str = status_display_name(entry.status);
		auto value_str = QString::fromStdString(entry.value);
		if (value_str.length() > max_value_display_length)
			value_str = value_str.left(max_value_display_length) + "...";

		auto * item = new QListWidgetItem(
		    QString("[%1] (%2) %3").arg(timestamp_str, status_str, value_str), m_list);
		item->setData(history_index_role, static_cast<qulonglong>(history_index));
	}

	if (m_list->count() > 0)
		m_list->setCurrentRow(0);
}

void history_view_t::clear()
{
	m_list->clear();
	m_allow_revert = false;
}

void history_view_t::on_context_menu(const QPoint & pos)
{
	if (!m_allow_revert)
		return;

	auto * item = m_list->itemAt(pos);
	if (!item)
		return;

	QMenu menu;
	auto * revert_action = menu.addAction(tr("Revert"));
	revert_action->setToolTip(tr("Restore the selected earlier translation"));

	if (menu.exec(m_list->viewport()->mapToGlobal(pos)) != revert_action)
		return;

	const auto history_index = static_cast<size_t>(item->data(history_index_role).toULongLong());
	emit revert_requested(history_index);
}
