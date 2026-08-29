#pragma once

#include "nav_tree_filter.hpp"
#include <io/codepage.hpp>
#include <scanner/plugin_scan.hpp>
#include <conflict_types.hpp>
#include <set>
#include <string>
#include <vector>
#include <QAbstractItemModel>
#include <QMimeData>

class editable_column_set_t;

class nav_tree_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit nav_tree_model_t(plugin_scan_t & scan, QObject * parent = nullptr);

	void rebuild();
	void refresh_colors();
	void set_excluded_plugins(const std::set<std::string> * excluded);
	void set_patch_plugins(const std::set<std::string> * patch);
	void set_dirty_plugins(const std::set<std::string> * dirty);
	void set_editable_columns(const editable_column_set_t * editable);

	using filter_state_t = nav_tree_filter_t::filter_state_t;

	void set_filter(const filter_state_t & state);
	void clear_filter();
	void set_hide_duplicates(bool hide);
	void set_show_deleted_strikeout(bool value);
	void set_display_codepage(codepage_t codepage);

	void sort(int column, Qt::SortOrder order) override;

	QModelIndex index(int row, int column, const QModelIndex & parent) const override;
	QModelIndex parent(const QModelIndex & child) const override;
	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;
	QVariant data(const QModelIndex & index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex & index) const override;

	Qt::DropActions supportedDragActions() const override;
	QMimeData * mimeData(const QModelIndexList & indexes) const override;

	struct node_info_t
	{
		int plugin_idx = -1;
		std::string rec_type;
		std::string record_id;
	};

	node_info_t node_at(const QModelIndex & index) const;
	QModelIndex find_index(const std::string & rec_type, const std::string & record_id) const;

private:
	plugin_scan_t & m_scan;

	struct visible_record_t
	{
		size_t entry_idx;
	};

	struct type_group_t
	{
		std::string type;
		std::vector<visible_record_t> records;
	};

	struct file_node_t
	{
		int plugin_idx;
		std::vector<type_group_t> groups;
	};

	std::vector<file_node_t> m_tree;
	nav_tree_filter_t m_filter;
	int m_sort_column = 1;
	Qt::SortOrder m_sort_order = Qt::AscendingOrder;
	bool m_show_deleted_strikeout = false;
	codepage_t m_display_codepage = codepage_t::windows_1252;
	const editable_column_set_t * m_editable_columns = nullptr;

	conflict_this_t record_foreground_for_plugin(const conflict_entry_t & entry, int plugin_idx) const;

	void build_tree();
	void sort_records();

	QModelIndex index_for_root_level(int row, int column) const;
	QModelIndex index_for_esm_group(void * ptr, int parent_row, int row, int column) const;

	QVariant data_for_root_level(int row, int column, int role) const;
	QVariant data_for_file_node(int row, int column, int role) const;
	QVariant file_node_display_text(const file_node_t & file_node) const;
	QVariant file_node_appearance(const file_node_t & file_node, int role) const;
	QVariant data_for_esm_nodes(void * ptr, int row, int column, int role) const;
	QVariant data_for_type_group(size_t file_idx, int row, int column, int role) const;
	QVariant data_for_record(size_t file_idx, size_t group_idx, int row, int column, int role) const;
};
