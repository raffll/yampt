#include "lua_registrations_model.hpp"
#include <map>
#include <QString>

lua_registrations_model_t::lua_registrations_model_t(QObject * parent)
    : QAbstractItemModel(parent)
{}

void lua_registrations_model_t::set_registrations(const std::vector<handler_registration_t> & registrations)
{
	beginResetModel();
	m_all_registrations = registrations;
	rebuild_groups();
	endResetModel();
}

const handler_registration_t * lua_registrations_model_t::registration_at(const QModelIndex & index) const
{
	if (!index.isValid())
		return nullptr;

	const auto parent_index = index.parent();
	if (!parent_index.isValid())
		return nullptr;

	const auto grandparent = parent_index.parent();
	if (!grandparent.isValid())
		return nullptr;

	const auto mod_idx = static_cast<size_t>(grandparent.row());
	if (mod_idx >= m_mod_groups.size())
		return nullptr;

	const auto script_idx = static_cast<size_t>(parent_index.row());
	if (script_idx >= m_mod_groups[mod_idx].scripts.size())
		return nullptr;

	const auto & script_group = m_mod_groups[mod_idx].scripts[script_idx];
	const auto reg_idx = static_cast<size_t>(index.row());
	if (reg_idx >= script_group.registration_indices.size())
		return nullptr;

	return &m_all_registrations[script_group.registration_indices[reg_idx]];
}

QModelIndex lua_registrations_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (column != 0 || row < 0)
		return {};

	if (!parent.isValid())
	{
		if (row >= static_cast<int>(m_mod_groups.size()))
			return {};

		return createIndex(row, 0, quintptr(0));
	}

	if (!parent.parent().isValid())
	{
		const auto mod_idx = static_cast<size_t>(parent.row());
		if (mod_idx >= m_mod_groups.size())
			return {};

		if (row >= static_cast<int>(m_mod_groups[mod_idx].scripts.size()))
			return {};

		return createIndex(row, 0, quintptr(mod_idx + 1));
	}

	const auto mod_idx = static_cast<size_t>(parent.parent().row());
	if (mod_idx >= m_mod_groups.size())
		return {};

	const auto script_idx = static_cast<size_t>(parent.row());
	if (script_idx >= m_mod_groups[mod_idx].scripts.size())
		return {};

	const auto & script_group = m_mod_groups[mod_idx].scripts[script_idx];
	if (row >= static_cast<int>(script_group.registration_indices.size()))
		return {};

	const auto encoded = (mod_idx + 1) * 10000 + (script_idx + 1);
	return createIndex(row, 0, quintptr(encoded));
}

QModelIndex lua_registrations_model_t::parent(const QModelIndex & child) const
{
	if (!child.isValid())
		return {};

	const auto internal = child.internalId();

	if (internal == 0)
		return {};

	if (internal < 10000)
	{
		const auto mod_idx = static_cast<int>(internal - 1);
		return createIndex(mod_idx, 0, quintptr(0));
	}

	const auto mod_idx = static_cast<int>(internal / 10000) - 1;
	const auto script_idx = static_cast<int>(internal % 10000) - 1;

	return createIndex(script_idx, 0, quintptr(static_cast<size_t>(mod_idx) + 1));
}

int lua_registrations_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(m_mod_groups.size());

	const auto internal = parent.internalId();

	if (internal == 0)
	{
		const auto mod_idx = static_cast<size_t>(parent.row());
		if (mod_idx >= m_mod_groups.size())
			return 0;

		return static_cast<int>(m_mod_groups[mod_idx].scripts.size());
	}

	if (internal < 10000)
	{
		const auto mod_idx = static_cast<size_t>(internal - 1);
		if (mod_idx >= m_mod_groups.size())
			return 0;

		const auto script_idx = static_cast<size_t>(parent.row());
		if (script_idx >= m_mod_groups[mod_idx].scripts.size())
			return 0;

		return static_cast<int>(m_mod_groups[mod_idx].scripts[script_idx].registration_indices.size());
	}

	return 0;
}

int lua_registrations_model_t::columnCount(const QModelIndex &) const
{
	return 1;
}

QVariant lua_registrations_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return {};

	const auto internal = index.internalId();

	if (internal == 0)
	{
		const auto mod_idx = static_cast<size_t>(index.row());
		if (mod_idx >= m_mod_groups.size())
			return {};

		const auto & group = m_mod_groups[mod_idx];
		return QString::fromStdString(group.mod_name);
	}

	if (internal < 10000)
	{
		const auto mod_idx = static_cast<size_t>(internal - 1);
		if (mod_idx >= m_mod_groups.size())
			return {};

		const auto script_idx = static_cast<size_t>(index.row());
		if (script_idx >= m_mod_groups[mod_idx].scripts.size())
			return {};

		return QString::fromStdString(m_mod_groups[mod_idx].scripts[script_idx].script_path);
	}

	const auto mod_idx = static_cast<size_t>(internal / 10000) - 1;
	const auto script_idx = static_cast<size_t>(internal % 10000) - 1;

	if (mod_idx >= m_mod_groups.size())
		return {};

	if (script_idx >= m_mod_groups[mod_idx].scripts.size())
		return {};

	const auto & script_group = m_mod_groups[mod_idx].scripts[script_idx];
	const auto reg_idx = static_cast<size_t>(index.row());
	if (reg_idx >= script_group.registration_indices.size())
		return {};

	const auto & registration = m_all_registrations[script_group.registration_indices[reg_idx]];
	auto display_text = registration.interface_name + "." + registration.method_name;

	if (!registration.type_argument.empty())
		display_text += " [" + registration.type_argument + "]";

	return QString::fromStdString(display_text);
}

Qt::ItemFlags lua_registrations_model_t::flags(const QModelIndex & index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void lua_registrations_model_t::rebuild_groups()
{
	m_mod_groups.clear();

	std::map<std::string, std::map<std::string, std::vector<size_t>>> grouped;

	for (size_t idx = 0; idx < m_all_registrations.size(); ++idx)
	{
		const auto & registration = m_all_registrations[idx];
		grouped[registration.mod_name][registration.script_path].push_back(idx);
	}

	for (const auto & [mod_name, scripts] : grouped)
	{
		mod_group_t mod_group;
		mod_group.mod_name = mod_name;

		for (const auto & [script_path, indices] : scripts)
			mod_group.scripts.push_back({ script_path, indices });

		m_mod_groups.push_back(std::move(mod_group));
	}
}
