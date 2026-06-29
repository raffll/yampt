#pragma once

#include <plugin_scan/plugin_scan.hpp>
#include <plugin_scan/conflict_types.hpp>
#include <QAbstractItemModel>
#include <QMimeData>
#include <set>
#include <string>
#include <vector>

class nav_tree_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit nav_tree_model_t(plugin_scan_t & scan, QObject * parent = nullptr);

	void rebuild();

	struct filter_state_t
	{
		bool filter_conflict_all = false;
		std::set<conflict_all_t> conflict_all_set;
		bool filter_conflict_this = false;
		std::set<conflict_this_t> conflict_this_set;
		bool filter_by_type = false;
		std::set<std::string> type_set;
		bool filter_by_id = false;
		std::string id_text;
		bool filter_by_name = false;
		std::string name_text;
		bool filter_deleted = false;
		bool filter_itm_only = false;

		bool operator==(const filter_state_t &) const = default;
	};

	void set_filter(const filter_state_t & state);
	void clear_filter();

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
		int plugin_idx;
		std::string rec_type;
		std::string record_id;
	};

	node_info_t node_at(const QModelIndex & index) const;

private:
	plugin_scan_t & scan_;

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

	std::vector<file_node_t> tree_;

	filter_state_t filter_;
	bool has_filter_ = false;

	void build_tree();
	bool passes_filter(const conflict_entry_t & entry, int plugin_idx) const;
};
