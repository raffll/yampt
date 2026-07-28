#include "lua_nav_model.hpp"

#include <algorithm>
#include <map>
#include <QString>

lua_nav_model_t::lua_nav_model_t(QObject * parent)
    : QAbstractItemModel(parent)
{}

void lua_nav_model_t::set_conflicts(const std::vector<handler_conflict_t> & conflicts)
{
	beginResetModel();
	m_all_conflicts = conflicts;
	m_interface_filter.clear();
	for (const auto & conflict : m_all_conflicts)
		m_interface_filter.insert(conflict.interface_name);
	m_severity_filter = { true, true, true };
	rebuild_groups();
	endResetModel();
}

void lua_nav_model_t::set_filter(const severity_set_t & severities, const interface_set_t & interfaces)
{
	emit layoutAboutToBeChanged();
	m_severity_filter = severities;
	m_interface_filter = interfaces;
	rebuild_groups();
	emit layoutChanged();
}

const handler_conflict_t * lua_nav_model_t::conflict_at(const QModelIndex & index) const
{
	if (!index.isValid())
		return nullptr;

	const auto * group_ptr = static_cast<const interface_group_t *>(index.internalPointer());
	if (group_ptr == nullptr)
		return nullptr;

	int row = index.row();
	if (row < 0 || row >= static_cast<int>(group_ptr->conflict_indices.size()))
		return nullptr;

	size_t conflict_idx = group_ptr->conflict_indices[static_cast<size_t>(row)];
	return &m_all_conflicts[conflict_idx];
}

QModelIndex lua_nav_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (column != 0)
		return {};

	if (!parent.isValid())
	{
		if (row < 0 || row >= static_cast<int>(m_interface_groups.size()))
			return {};

		return createIndex(row, column, nullptr);
	}

	if (parent.internalPointer() != nullptr)
		return {};

	int group_idx = parent.row();
	if (group_idx < 0 || group_idx >= static_cast<int>(m_interface_groups.size()))
		return {};

	const auto & group = m_interface_groups[static_cast<size_t>(group_idx)];
	if (row < 0 || row >= static_cast<int>(group.conflict_indices.size()))
		return {};

	return createIndex(row, column, const_cast<interface_group_t *>(&group));
}

QModelIndex lua_nav_model_t::parent(const QModelIndex & child) const
{
	if (!child.isValid())
		return {};

	const auto * ptr = child.internalPointer();
	if (ptr == nullptr)
		return {};

	for (size_t idx = 0; idx < m_interface_groups.size(); ++idx)
	{
		if (ptr == &m_interface_groups[idx])
			return createIndex(static_cast<int>(idx), 0, nullptr);
	}

	return {};
}

int lua_nav_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(m_interface_groups.size());

	if (parent.internalPointer() != nullptr)
		return 0;

	int group_idx = parent.row();
	if (group_idx < 0 || group_idx >= static_cast<int>(m_interface_groups.size()))
		return 0;

	return static_cast<int>(m_interface_groups[static_cast<size_t>(group_idx)].conflict_indices.size());
}

int lua_nav_model_t::columnCount(const QModelIndex &) const
{
	return 1;
}

QVariant lua_nav_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return {};

	const auto * ptr = index.internalPointer();

	if (ptr == nullptr)
	{
		int row = index.row();
		if (row < 0 || row >= static_cast<int>(m_interface_groups.size()))
			return {};

		const auto & group = m_interface_groups[static_cast<size_t>(row)];
		int count = static_cast<int>(group.conflict_indices.size());
		return tr("%1 (%2)").arg(QString::fromStdString(group.interface_name)).arg(count);
	}

	const auto * group_ptr = static_cast<const interface_group_t *>(ptr);
	int row = index.row();
	if (row < 0 || row >= static_cast<int>(group_ptr->conflict_indices.size()))
		return {};

	size_t conflict_idx = group_ptr->conflict_indices[static_cast<size_t>(row)];
	const auto & conflict = m_all_conflicts[conflict_idx];

	if (conflict.type_argument.empty())
		return QString::fromStdString(conflict.method_name);

	return QString::fromStdString(conflict.method_name + " [" + conflict.type_argument + "]");
}

Qt::ItemFlags lua_nav_model_t::flags(const QModelIndex & index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void lua_nav_model_t::rebuild_groups()
{
	m_interface_groups.clear();

	std::map<std::string, std::vector<size_t>> grouped;
	for (size_t idx = 0; idx < m_all_conflicts.size(); ++idx)
	{
		const auto & conflict = m_all_conflicts[idx];

		if (!passes_severity(conflict.severity))
			continue;

		if (m_interface_filter.find(conflict.interface_name) == m_interface_filter.end())
			continue;

		grouped[conflict.interface_name].push_back(idx);
	}

	for (auto & [name, indices] : grouped)
	{
		std::sort(indices.begin(), indices.end(), [this](size_t lhs, size_t rhs)
		{
			return severity_order(m_all_conflicts[lhs].severity) < severity_order(m_all_conflicts[rhs].severity);
		});

		m_interface_groups.push_back({ name, std::move(indices) });
	}

	std::sort(m_interface_groups.begin(), m_interface_groups.end(), [](const auto & lhs, const auto & rhs)
	{
		return lhs.interface_name < rhs.interface_name;
	});
}

bool lua_nav_model_t::passes_severity(conflict_severity_t severity) const
{
	switch (severity)
	{
	case conflict_severity_t::blocking:
		return m_severity_filter.blocking;

	case conflict_severity_t::mutating:
		return m_severity_filter.mutating;

	case conflict_severity_t::overlapping:
		return m_severity_filter.overlapping;
	}

	return false;
}

int lua_nav_model_t::severity_order(conflict_severity_t severity)
{
	switch (severity)
	{
	case conflict_severity_t::blocking:
		return 0;

	case conflict_severity_t::mutating:
		return 1;

	case conflict_severity_t::overlapping:
		return 2;
	}

	return 3;
}
