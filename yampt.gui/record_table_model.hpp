#pragma once

#include "../yampt/tools.hpp"
#include <QAbstractTableModel>
#include <string>
#include <vector>

enum table_col_t
{
	col_id = 0,
	col_key = 1,
	col_original = 2,
	col_translation = 3,
	col_status = 4,
	col_count = 5
};

struct table_row_t
{
	tools_t::rec_type_t type;
	std::string key_text;
	bool is_child = false;
	std::string old_text;
	std::string new_text;
	std::string status;
	size_t chapter_index;
};

class record_table_model_t : public QAbstractTableModel
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
	const table_row_t * row_at(int row) const;
	void update_row(int row, const std::string & new_text, const std::string & status);

private:
	std::vector<table_row_t> rows_;
};
