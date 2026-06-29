#pragma once

#include <vector>
#include <QTableView>

class record_table_view_t : public QTableView
{
	Q_OBJECT

public:
	explicit record_table_view_t(QWidget * parent = nullptr);

	void setModel(QAbstractItemModel * model) override;
	void set_column_widths(const std::vector<int> & widths);
	std::vector<int> get_column_widths() const;

signals:
	void row_selected(int row);
	void batch_status_change_requested(const QList<int> & rows, const QString & new_status);

protected:
	void contextMenuEvent(QContextMenuEvent * event) override;
};
