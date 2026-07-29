#pragma once

#include <scanner/conflict_detector.hpp>
#include <set>
#include <string>
#include <vector>
#include <QAbstractItemModel>

class lua_nav_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit lua_nav_model_t(QObject * parent = nullptr);

	struct severity_set_t
	{
		bool blocking = true;
		bool mutating = true;
		bool overlapping = true;
	};

	using interface_set_t = std::set<std::string>;

	void set_conflicts(const std::vector<handler_conflict_t> & conflicts);
	void set_filter(const severity_set_t & severities, const interface_set_t & interfaces);

	const handler_conflict_t * conflict_at(const QModelIndex & index) const;

	QModelIndex index(int row, int column, const QModelIndex & parent) const override;
	QModelIndex parent(const QModelIndex & child) const override;
	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;
	QVariant data(const QModelIndex & index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex & index) const override;

private:
	struct interface_group_t
	{
		std::string interface_name;
		std::vector<size_t> conflict_indices;
	};

	std::vector<handler_conflict_t> m_all_conflicts;
	std::vector<interface_group_t> m_interface_groups;

	severity_set_t m_severity_filter;
	interface_set_t m_interface_filter;

	void rebuild_groups();
	bool passes_severity(conflict_severity_t severity) const;
	static int severity_order(conflict_severity_t severity);
};
