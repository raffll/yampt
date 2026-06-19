#pragma once

#include "../yampt/plugin_scan/plugin_scan.hpp"
#include "../yampt/plugin_scan/conflict_types.hpp"
#include "../yampt/plugin_scan/sub_record_iter.hpp"
#include "../yampt/plugin_scan/sub_record_schema.hpp"
#include <QAbstractItemModel>
#include <QMimeData>
#include <string>
#include <vector>

class view_tree_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit view_tree_model_t(QObject * parent = nullptr);

	void set_record(plugin_scan_t & scan, const conflict_entry_t & entry);
	void clear();
	void set_hide_no_conflict(bool hide);
	bool is_merge_column(int section) const;
	int merge_column() const;

	QModelIndex index(int row, int column, const QModelIndex & parent) const override;
	QModelIndex parent(const QModelIndex & child) const override;
	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;
	QVariant data(const QModelIndex & index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex & index) const override;
	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;
	QMimeData * mimeData(const QModelIndexList & indexes) const override;
	bool canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
	    const override;

private:
	struct field_row_t
	{
		std::string name;
		std::vector<std::string> values;
		std::vector<conflict_this_t> cell_conflict_this;
		conflict_all_t row_conflict_all;
		bool all_identical;
	};

	struct sub_record_row_t
	{
		std::string type;
		std::string label;
		size_t size;
		std::vector<std::string> values;
		std::vector<conflict_this_t> cell_conflict_this;
		conflict_all_t row_conflict_all;
		bool all_identical;
		std::vector<field_row_t> children;
	};

	std::vector<sub_record_row_t> rows_;
	std::vector<std::string> column_names_;
	std::vector<conflict_this_t> plugin_conflict_this_;
	bool hide_no_conflict_ = false;
	bool has_merge_column_ = false;
	int merge_col_index_ = -1;
	std::string record_type_;
	std::string record_id_;
	std::vector<int> column_plugin_indices_;

	std::string format_value(const char * data, size_t size) const;
	std::string decode_field(const field_def_t & field, const char * data, size_t data_size) const;
	std::string make_sub_label(const std::string & sub_type, const std::string & record_type, size_t data_size) const;
	conflict_all_t compute_row_conflict_all(const std::vector<std::string> & values) const;
	std::vector<conflict_this_t> compute_row_conflict_this(const std::vector<std::string> & values) const;
	const std::vector<sub_record_row_t> & visible_rows() const;
	mutable std::vector<sub_record_row_t> filtered_rows_;
	mutable bool filter_dirty_ = true;
};
