#pragma once

#include <scanner/handler_parser.hpp>
#include <string>
#include <vector>
#include <QAbstractItemModel>

class lua_registrations_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit lua_registrations_model_t(QObject * parent = nullptr);

	void set_registrations(const std::vector<handler_registration_t> & registrations);
	const handler_registration_t * registration_at(const QModelIndex & index) const;

	QModelIndex index(int row, int column, const QModelIndex & parent) const override;
	QModelIndex parent(const QModelIndex & child) const override;
	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;
	QVariant data(const QModelIndex & index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex & index) const override;

private:
	struct script_group_t
	{
		std::string script_path;
		std::vector<size_t> registration_indices;
	};

	struct mod_group_t
	{
		std::string mod_name;
		std::vector<script_group_t> scripts;
	};

	std::vector<handler_registration_t> m_all_registrations;
	std::vector<mod_group_t> m_mod_groups;

	void rebuild_groups();
};
