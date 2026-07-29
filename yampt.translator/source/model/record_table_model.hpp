#pragma once

#include "row_source.hpp"
#include "table_row.hpp"
#include <vector>
#include <QAbstractTableModel>

class record_table_model_t : public QAbstractTableModel, public row_source_t
{
	Q_OBJECT

public:
	using QAbstractTableModel::QAbstractTableModel;

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	int columnCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

	void rebuild(std::vector<table_row_t> rows);
	const table_row_t * row_at(int row) const override;
	int row_count() const override;
	void update_row(int row, const std::string & new_text, status_t status);
	void set_editable(bool editable);

	Qt::ItemFlags flags(const QModelIndex & index) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

signals:
	void inline_edit_committed(int row, const std::string & new_text);

private:
	std::vector<table_row_t> m_rows;
	bool m_editable = true;
};
