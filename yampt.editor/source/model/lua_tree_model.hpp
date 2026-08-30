#pragma once

#include <scanner/conflict_detector.hpp>
#include <scanner/lua_scanner.hpp>
#include <string>
#include <vector>
#include <QAbstractItemModel>

class lua_tree_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit lua_tree_model_t(QObject * parent = nullptr);

	void set_scan_result(const lua_scan_result_t & result);
	void clear();

	struct node_info_t
	{
		bool is_group = false;
		int registration_idx = -1;
	};

	node_info_t node_at(const QModelIndex & index) const;

	QModelIndex index(int row, int column, const QModelIndex & parent) const override;
	QModelIndex parent(const QModelIndex & child) const override;
	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;
	QVariant data(const QModelIndex & index, int role) const override;

	bool has_conflicts() const;
	const lua_scan_result_t & scan_result() const;

private:
	struct lua_group_t
	{
		std::string group_name;
		std::vector<size_t> leaf_indices;
	};

	lua_scan_result_t m_scan_result;
	std::vector<lua_group_t> m_groups;
	bool m_conflicts_mode = false;

	void build_groups();
	bool is_group_pointer(void * ptr) const;
	int group_row_from_pointer(void * ptr) const;

	QVariant data_for_group(int row, int column, int role) const;
	QVariant data_for_leaf(void * ptr, int row, int column, int role) const;
	QVariant leaf_display_text(size_t leaf_idx) const;

	conflict_severity_t find_severity_for_registration(const handler_registration_t & registration) const;
	bool is_in_conflict(const handler_registration_t & registration) const;
};
