#pragma once

#include "../model/edit_history.hpp"
#include <vector>
#include <QWidget>

class QVBoxLayout;

class history_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit history_view_t(QWidget * parent = nullptr);

	void update_history(const std::vector<edit_history_entry_t> & entries);
	void clear();

private:
	QWidget * m_scroll_content = nullptr;
	QVBoxLayout * m_entries_layout = nullptr;
};
