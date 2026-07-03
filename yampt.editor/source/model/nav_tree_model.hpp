#pragma once

#include <conflict_types.hpp>
#include <scanner/plugin_scan.hpp>
#include <set>
#include <string>
#include <vector>
#include <QAbstractItemModel>
#include <QMimeData>

class nav_tree_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit nav_tree_model_t(plugin_scan_t & scan, QObject * parent = nullptr);

	void rebuild();
	void set_excluded_plugins(const std::set<std::string> * excluded);
	void set_patch_plugins(const std::set<std::string> * patch);

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
	void set_hide_duplicates(bool hide);

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

	filter_state_t m_filter;
	bool m_has_filter = false;
	bool m_hide_duplicates = false;
	const std::set<std::string> * m_excluded_plugins = nullptr;
	const std::set<std::string> * m_patch_plugins = nullptr;

	conflict_this_t record_foreground_for_plugin(const conflict_entry_t & entry, int plugin_idx) const;

	void build_tree();
	bool passes_filter(const conflict_entry_t & entry, int plugin_idx) const;
};
