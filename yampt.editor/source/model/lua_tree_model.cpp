#include "lua_tree_model.hpp"
#include <map>
#include <set>
#include <QBrush>
#include <QColor>
#include <QString>

lua_tree_model_t::lua_tree_model_t(QObject * parent)
    : QAbstractItemModel(parent)
{}

void lua_tree_model_t::set_scan_result(const lua_scan_result_t & result)
{
	beginResetModel();
	m_scan_result = result;
	m_conflicts_mode = !result.conflicts.empty();
	build_groups();
	endResetModel();
}

void lua_tree_model_t::clear()
{
	beginResetModel();
	m_scan_result.conflicts.clear();
	m_scan_result.registrations.clear();
	m_scan_result.warnings.clear();
	m_groups.clear();
	m_conflicts_mode = false;
	endResetModel();
}

bool lua_tree_model_t::has_conflicts() const
{
	return m_conflicts_mode;
}

const lua_scan_result_t & lua_tree_model_t::scan_result() const
{
	return m_scan_result;
}

void lua_tree_model_t::build_groups()
{
	m_groups.clear();

	std::set<size_t> conflicting_indices;
	for (const auto & conflict : m_scan_result.conflicts)
	{
		for (const auto & reg : conflict.registrations)
		{
			for (size_t idx = 0; idx < m_scan_result.registrations.size(); ++idx)
			{
				const auto & candidate = m_scan_result.registrations[idx];
				if (candidate.script_path == reg.script_path && candidate.line_number == reg.line_number)
					conflicting_indices.insert(idx);
			}
		}
	}

	const bool filter_to_conflicts = m_conflicts_mode;

	std::map<std::string, std::vector<size_t>> groups_by_mod;

	for (size_t idx = 0; idx < m_scan_result.registrations.size(); ++idx)
	{
		if (filter_to_conflicts && conflicting_indices.find(idx) == conflicting_indices.end())
			continue;

		const auto & registration = m_scan_result.registrations[idx];
		groups_by_mod[registration.mod_name].push_back(idx);
	}

	for (auto & [mod_name, indices] : groups_by_mod)
	{
		lua_group_t group;
		group.group_name = mod_name;
		group.leaf_indices = std::move(indices);
		m_groups.push_back(std::move(group));
	}
}

QModelIndex lua_tree_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (column < 0 || column >= 1)
		return {};

	if (!parent.isValid())
	{
		if (row < 0 || row >= static_cast<int>(m_groups.size()))
			return {};

		return createIndex(row, column, nullptr);
	}

	void * ptr = parent.internalPointer();
	if (ptr != nullptr)
		return {};

	int group_row = parent.row();
	if (group_row < 0 || group_row >= static_cast<int>(m_groups.size()))
		return {};

	const auto & group = m_groups[static_cast<size_t>(group_row)];
	if (row < 0 || row >= static_cast<int>(group.leaf_indices.size()))
		return {};

	return createIndex(row, column, const_cast<lua_group_t *>(&group));
}

QModelIndex lua_tree_model_t::parent(const QModelIndex & child) const
{
	if (!child.isValid())
		return {};

	void * ptr = child.internalPointer();
	if (ptr == nullptr)
		return {};

	int group_row = group_row_from_pointer(ptr);
	if (group_row < 0)
		return {};

	return createIndex(group_row, 0, nullptr);
}

int lua_tree_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(m_groups.size());

	void * ptr = parent.internalPointer();
	if (ptr != nullptr)
		return 0;

	int group_row = parent.row();
	if (group_row < 0 || group_row >= static_cast<int>(m_groups.size()))
		return 0;

	return static_cast<int>(m_groups[static_cast<size_t>(group_row)].leaf_indices.size());
}

int lua_tree_model_t::columnCount(const QModelIndex &) const
{
	return 1;
}

QVariant lua_tree_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};

	void * ptr = index.internalPointer();

	if (ptr == nullptr)
		return data_for_group(index.row(), index.column(), role);

	return data_for_leaf(ptr, index.row(), index.column(), role);
}

lua_tree_model_t::node_info_t lua_tree_model_t::node_at(const QModelIndex & index) const
{
	if (!index.isValid())
		return {};

	void * ptr = index.internalPointer();

	if (ptr == nullptr)
		return { true, -1 };

	int group_row = group_row_from_pointer(ptr);
	if (group_row < 0)
		return {};

	const auto & group = m_groups[static_cast<size_t>(group_row)];
	int row = index.row();
	if (row < 0 || row >= static_cast<int>(group.leaf_indices.size()))
		return {};

	size_t leaf_idx = group.leaf_indices[static_cast<size_t>(row)];
	return { false, static_cast<int>(leaf_idx) };
}

bool lua_tree_model_t::is_group_pointer(void * ptr) const
{
	for (size_t idx = 0; idx < m_groups.size(); ++idx)
	{
		if (ptr == &m_groups[idx])
			return true;
	}

	return false;
}

int lua_tree_model_t::group_row_from_pointer(void * ptr) const
{
	for (size_t idx = 0; idx < m_groups.size(); ++idx)
	{
		if (ptr == &m_groups[idx])
			return static_cast<int>(idx);
	}

	return -1;
}

QVariant lua_tree_model_t::data_for_group(int row, int column, int role) const
{
	if (row < 0 || row >= static_cast<int>(m_groups.size()))
		return {};

	if (role != Qt::DisplayRole || column != 0)
		return {};

	const auto & group = m_groups[static_cast<size_t>(row)];
	return QString("%1 [%2]")
	    .arg(QString::fromStdString(group.group_name))
	    .arg(group.leaf_indices.size());
}

QVariant lua_tree_model_t::data_for_leaf(void * ptr, int row, int column, int role) const
{
	int group_row = group_row_from_pointer(ptr);
	if (group_row < 0)
		return {};

	const auto & group = m_groups[static_cast<size_t>(group_row)];
	if (row < 0 || row >= static_cast<int>(group.leaf_indices.size()))
		return {};

	size_t leaf_idx = group.leaf_indices[static_cast<size_t>(row)];

	if (role == Qt::DisplayRole && column == 0)
		return leaf_display_text(leaf_idx);

	if (role == Qt::ForegroundRole && m_conflicts_mode)
	{
		if (leaf_idx >= m_scan_result.registrations.size())
			return {};

		const auto & registration = m_scan_result.registrations[leaf_idx];
		if (!is_in_conflict(registration))
			return {};

		const auto severity = find_severity_for_registration(registration);

		switch (severity)
		{
		case conflict_severity_t::blocking:
			return QBrush(QColor(200, 50, 50));

		case conflict_severity_t::mutating:
			return QBrush(QColor(180, 120, 0));

		case conflict_severity_t::overlapping:
			return QBrush(QColor(100, 140, 0));
		}
	}

	return {};
}

QVariant lua_tree_model_t::leaf_display_text(size_t leaf_idx) const
{
	if (leaf_idx >= m_scan_result.registrations.size())
		return {};

	const auto & registration = m_scan_result.registrations[leaf_idx];

	if (registration.type_argument.empty())
	{
		return QString("%1.%2")
		    .arg(QString::fromStdString(registration.interface_name))
		    .arg(QString::fromStdString(registration.method_name));
	}

	return QString("%1.%2 [%3]")
	    .arg(QString::fromStdString(registration.interface_name))
	    .arg(QString::fromStdString(registration.method_name))
	    .arg(QString::fromStdString(registration.type_argument));
}

conflict_severity_t lua_tree_model_t::find_severity_for_registration(
    const handler_registration_t & registration) const
{
	for (const auto & conflict : m_scan_result.conflicts)
	{
		for (const auto & reg : conflict.registrations)
		{
			if (reg.script_path == registration.script_path && reg.line_number == registration.line_number)
				return conflict.severity;
		}
	}

	return conflict_severity_t::overlapping;
}

bool lua_tree_model_t::is_in_conflict(const handler_registration_t & registration) const
{
	for (const auto & conflict : m_scan_result.conflicts)
	{
		for (const auto & reg : conflict.registrations)
		{
			if (reg.script_path == registration.script_path && reg.line_number == registration.line_number)
				return true;
		}
	}

	return false;
}
