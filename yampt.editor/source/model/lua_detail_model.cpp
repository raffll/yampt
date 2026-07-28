#include "lua_detail_model.hpp"

#include <QBrush>
#include <QColor>
#include <QString>

lua_detail_model_t::lua_detail_model_t(QObject * parent)
    : QAbstractItemModel(parent)
{}

void lua_detail_model_t::set_conflict(const handler_conflict_t & conflict)
{
	beginResetModel();
	m_registrations = conflict.registrations;
	m_column_names.clear();
	for (const auto & registration : m_registrations)
		m_column_names.push_back(registration.mod_name);
	rebuild_rows();
	endResetModel();
}

void lua_detail_model_t::set_registration(const handler_registration_t & registration)
{
	beginResetModel();
	m_registrations = { registration };
	m_column_names = { registration.mod_name };
	rebuild_rows();
	endResetModel();
}

void lua_detail_model_t::clear()
{
	beginResetModel();
	m_registrations.clear();
	m_column_names.clear();
	m_field_rows.clear();
	endResetModel();
}

void lua_detail_model_t::rebuild_rows()
{
	m_field_rows.clear();
	m_field_rows.push_back({ "Script", field_id_t::script_path });
	m_field_rows.push_back({ "Line", field_id_t::line_number });
	m_field_rows.push_back({ "Interface", field_id_t::interface_name });
	m_field_rows.push_back({ "Method", field_id_t::method_name });

	bool has_type = false;
	for (const auto & registration : m_registrations)
	{
		if (!registration.type_argument.empty())
			has_type = true;
	}

	if (has_type)
		m_field_rows.push_back({ "Type", field_id_t::type_argument });

	m_field_rows.push_back({ "Classification", field_id_t::classification });

	bool has_condition = false;
	for (const auto & registration : m_registrations)
	{
		if (!registration.blocking_condition.empty())
			has_condition = true;
	}

	if (has_condition)
		m_field_rows.push_back({ "Blocking Condition", field_id_t::blocking_condition });

	m_field_rows.push_back({ "Handler Body", field_id_t::handler_body });
}

QModelIndex lua_detail_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (parent.isValid())
		return {};

	if (row < 0 || row >= static_cast<int>(m_field_rows.size()))
		return {};

	if (column < 0 || column > static_cast<int>(m_registrations.size()))
		return {};

	return createIndex(row, column, nullptr);
}

QModelIndex lua_detail_model_t::parent(const QModelIndex &) const
{
	return {};
}

int lua_detail_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;

	return static_cast<int>(m_field_rows.size());
}

int lua_detail_model_t::columnCount(const QModelIndex &) const
{
	return static_cast<int>(m_registrations.size()) + 1;
}

QVariant lua_detail_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};

	const auto row_idx = static_cast<size_t>(index.row());
	if (row_idx >= m_field_rows.size())
		return {};

	if (index.column() == 0)
	{
		if (role == Qt::DisplayRole)
			return QString::fromStdString(m_field_rows[row_idx].label);

		return {};
	}

	const auto col_idx = static_cast<size_t>(index.column() - 1);
	if (col_idx >= m_registrations.size())
		return {};

	const auto & registration = m_registrations[col_idx];
	const auto & field = m_field_rows[row_idx];

	if (role == Qt::DisplayRole)
		return value_for_field(registration, field.field_id);

	if (role != Qt::BackgroundRole)
		return {};

	if (field.field_id != field_id_t::classification)
		return {};

	switch (registration.classification)
	{
	case handler_class_t::blocking:
		return QBrush(QColor(224, 82, 82, 60));

	case handler_class_t::mutating:
		return QBrush(QColor(224, 138, 62, 60));

	case handler_class_t::passive:
		return {};
	}

	return {};
}

QVariant lua_detail_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	if (section == 0)
		return tr("Field");

	const auto col_idx = static_cast<size_t>(section - 1);
	if (col_idx >= m_column_names.size())
		return {};

	return QString::fromStdString(m_column_names[col_idx]);
}

QString lua_detail_model_t::value_for_field(
    const handler_registration_t & registration, field_id_t field) const
{
	switch (field)
	{
	case field_id_t::mod_name:
		return QString::fromStdString(registration.mod_name);

	case field_id_t::script_path:
		return QString::fromStdString(registration.script_path);

	case field_id_t::line_number:
		return QString::number(registration.line_number);

	case field_id_t::interface_name:
		return QString::fromStdString(registration.interface_name);

	case field_id_t::method_name:
		return QString::fromStdString(registration.method_name);

	case field_id_t::type_argument:
		return QString::fromStdString(registration.type_argument);

	case field_id_t::classification:
		return classification_text(registration.classification);

	case field_id_t::blocking_condition:
		return QString::fromStdString(registration.blocking_condition);

	case field_id_t::handler_body:
		return QString::fromStdString(registration.handler_body);
	}

	return {};
}

QString lua_detail_model_t::classification_text(handler_class_t classification) const
{
	switch (classification)
	{
	case handler_class_t::blocking:
		return tr("Blocking");

	case handler_class_t::mutating:
		return tr("Mutating");

	case handler_class_t::passive:
		return tr("Passive");
	}

	return {};
}
