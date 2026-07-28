#pragma once

#include <scanner/conflict_detector.hpp>

#include <string>
#include <vector>
#include <QAbstractItemModel>

class lua_detail_model_t : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit lua_detail_model_t(QObject * parent = nullptr);

	void set_conflict(const handler_conflict_t & conflict);
	void set_registration(const handler_registration_t & registration);
	void clear();

	QModelIndex index(int row, int column, const QModelIndex & parent) const override;
	QModelIndex parent(const QModelIndex & child) const override;
	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;
	QVariant data(const QModelIndex & index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
	enum class field_id_t
	{
		mod_name,
		script_path,
		line_number,
		interface_name,
		method_name,
		type_argument,
		classification,
		blocking_condition,
		handler_body
	};

	struct field_row_t
	{
		std::string label;
		field_id_t field_id;
	};

	std::vector<handler_registration_t> m_registrations;
	std::vector<std::string> m_column_names;
	std::vector<field_row_t> m_field_rows;

	void rebuild_rows();
	QString value_for_field(const handler_registration_t & registration, field_id_t field) const;
	QString classification_text(handler_class_t classification) const;
};
