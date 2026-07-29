#pragma once

#include <utility/domain_types.hpp>
#include <vector>
#include <QWidget>

struct history_entry_t;

class QVBoxLayout;

class history_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit history_view_t(QWidget * parent = nullptr);

	void update_history(const std::vector<history_entry_t> & entries, bool allow_revert);
	void clear();

signals:
	void revert_requested(size_t history_index);

private:
	QVBoxLayout * m_entries_layout = nullptr;
	QWidget * m_scroll_content = nullptr;
};
