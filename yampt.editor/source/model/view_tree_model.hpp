#pragma once

#include <decoder/sub_record_iter.hpp>
#include <decoder/sub_record_schema.hpp>
#include <scanner/conflict_compute.hpp>
#include <scanner/conflict_types.hpp>
#include <scanner/plugin_scan.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <QAbstractItemModel>
#include <QMimeData>

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

	struct sub_slot_t
	{
		std::string type;
		int occurrence;
	};

private:
	struct record_context_t
	{
		std::vector<std::vector<sub_record_view_t>> & all_sub_records;
		std::vector<std::string> & content_storage;
		size_t col_count;
	};

	struct slot_build_context_t
	{
		std::vector<sub_slot_t> & unified_slots;
		std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_type_indices;
	};

	size_t setup_columns(plugin_scan_t & scan, const conflict_entry_t & entry);
	void setup_merge_column(plugin_scan_t & scan, const conflict_entry_t & entry, size_t & col_count);
	void build_header_row(plugin_scan_t & scan, const conflict_entry_t & entry);
	void load_sub_records(plugin_scan_t & scan, const conflict_entry_t & entry, record_context_t & context);

	void set_record_cell(record_context_t & context);
	void set_record_leveled(record_context_t & context, const conflict_entry_t & entry);
	void set_record_faction(record_context_t & context, const conflict_entry_t & entry);
	void set_record_container(record_context_t & context, const conflict_entry_t & entry);
	void set_record_generic(record_context_t & context, const conflict_entry_t & entry);

	void collect_leveled_entries(record_context_t & context, slot_build_context_t & build_ctx);
	void collect_faction_entries(record_context_t & context, slot_build_context_t & build_ctx);
	void collect_container_entries(record_context_t & context, slot_build_context_t & build_ctx);

	void decode_schema_children(
	    sub_record_row_t & parent_row,
	    const sub_record_schema_t * schema,
	    const char * first_data,
	    size_t first_size,
	    size_t col_count,
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
	    const sub_slot_t & slot);

	void decode_hex_children(
	    sub_record_row_t & parent_row,
	    size_t first_size,
	    size_t col_count,
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
	    const sub_slot_t & slot);

	void decode_schema_children_ref(
	    sub_record_row_t & parent_row,
	    const sub_record_schema_t * schema,
	    const char * first_data,
	    size_t first_size,
	    size_t col_count,
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    const std::vector<std::vector<struct cell_ref_group_t>> & col_refs,
	    uint32_t object_index,
	    const sub_slot_t & slot);

	void decode_hex_children_ref(
	    sub_record_row_t & parent_row,
	    size_t first_size,
	    size_t col_count,
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    const std::vector<std::vector<struct cell_ref_group_t>> & col_refs,
	    uint32_t object_index,
	    const sub_slot_t & slot);

	sub_record_row_t build_slot_row(
	    size_t col_count,
	    const std::vector<std::vector<sub_record_view_t>> & all_subs,
	    const std::vector<std::unordered_map<std::string, std::vector<size_t>>> & col_indices,
	    const sub_slot_t & slot);

	void emit_slot_rows(record_context_t & context, slot_build_context_t & build_ctx);

	void finalize_header_conflict();

	std::vector<sub_record_row_t> m_rows;
	std::vector<std::string> m_column_names;
	std::vector<conflict_this_t> m_plugin_conflict_this;
	bool m_hide_no_conflict = false;
	bool m_has_merge_column = false;
	int m_merge_col_index = -1;
	std::string m_record_type;
	std::string m_record_id;
	std::vector<int> m_column_plugin_indices;

	const std::vector<sub_record_row_t> & visible_rows() const;
	mutable std::vector<sub_record_row_t> m_filtered_rows;
	mutable bool m_filter_dirty = true;
};

struct cell_ref_group_t
{
	uint32_t object_index;
	size_t start_idx;
	size_t end_idx;
};
