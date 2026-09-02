#pragma once

#include "../model/edit_log.hpp"
#include <vector>
#include <QWidget>

class QListWidget;

class history_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit history_view_t(QWidget * parent = nullptr);

	void update_history(const std::vector<edit_log_entry_t> & entries);
	void clear();

private:
	QListWidget * m_list = nullptr;
};
