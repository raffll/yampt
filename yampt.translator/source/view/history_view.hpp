#pragma once

#include <utility/domain_types.hpp>
#include <vector>
#include <QWidget>

struct history_entry_t;

class QListWidget;

class history_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit history_view_t(QWidget * parent = nullptr);

	void update_history(const std::vector<history_entry_t> & entries, bool allow_revert);
	void clear();

signals:
	void revert_requested(size_t history_index);

private slots:
	void on_context_menu(const QPoint & pos);

private:
	QListWidget * m_list = nullptr;
	bool m_allow_revert = false;
};
