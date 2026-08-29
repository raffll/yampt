#pragma once

#include "../model/table_row.hpp"
#include <utility/status_types.hpp>
#include <functional>
#include <vector>
#include <QTableView>

class QKeyEvent;

class record_table_view_t : public QTableView
{
	Q_OBJECT

public:
	explicit record_table_view_t(QWidget * parent = nullptr);

	void setModel(QAbstractItemModel * model) override;
	void refresh_column_layout();
	void set_column_widths(const std::vector<int> & widths);
	std::vector<int> get_column_widths() const;
	void set_context_menu_enabled(bool enabled);
	void set_example_state_fn(std::function<bool(int row)> fn);
	void set_example_count_fn(std::function<int()> fn);
	void set_can_revert_fn(std::function<bool(int row)> fn);

signals:
	void row_selected(int row);
	void batch_status_change_requested(const QList<int> & rows, status_t new_status);
	void batch_revert_requested(const QList<int> & rows);
	void toggle_example_requested(const QList<int> & rows);
	void delete_entry_requested();

protected:
	void contextMenuEvent(QContextMenuEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;

private:
	void apply_column_layout();
	static int default_column_width(table_col_t logical_column);

	bool m_context_menu_enabled = true;
	std::function<bool(int row)> m_example_state_fn;
	std::function<int()> m_example_count_fn;
	std::function<bool(int row)> m_can_revert_fn;
};
