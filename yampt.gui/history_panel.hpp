#pragma once

#include "../yampt/tools.hpp"
#include <QWidget>
#include <vector>

struct history_entry_t;

class QVBoxLayout;

class history_panel_t : public QWidget
{
	Q_OBJECT

public:
	explicit history_panel_t(QWidget * parent = nullptr);

	void update_history(const std::vector<history_entry_t> & entries, bool allow_revert);
	void clear();

signals:
	void revert_requested(size_t history_index);

private:
	QVBoxLayout * entries_layout_ = nullptr;
	QWidget * scroll_content_ = nullptr;
};
