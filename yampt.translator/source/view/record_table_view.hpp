#pragma once

#include <utility/status_types.hpp>
#include <vector>
#include <QTableView>

class QKeyEvent;

class record_table_view_t : public QTableView
{
	Q_OBJECT

public:
	explicit record_table_view_t(QWidget * parent = nullptr);

	void setModel(QAbstractItemModel * model) override;
	void set_column_widths(const std::vector<int> & widths);
	std::vector<int> get_column_widths() const;
	void set_context_menu_enabled(bool enabled);

signals:
	void row_selected(int row);
	void batch_status_change_requested(const QList<int> & rows, status_t new_status);
	void delete_entry_requested();

protected:
	void contextMenuEvent(QContextMenuEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;

private:
	bool m_context_menu_enabled = true;
};
