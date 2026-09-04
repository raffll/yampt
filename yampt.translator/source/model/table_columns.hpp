#pragma once

#include "document.hpp"
#include "table_row.hpp"
#include <vector>
#include <QString>

class table_columns_t
{
public:
	table_columns_t();
	explicit table_columns_t(document_kind_t kind);

	void set_for_kind(document_kind_t kind);

	int count() const;
	table_col_t at(int visual_position) const;
	int position_of(table_col_t logical_column) const;
	bool contains(table_col_t logical_column) const;

	QString label_for(table_col_t logical_column) const;

	static std::vector<table_col_t> columns_for_kind(document_kind_t kind);

private:
	QString label_original() const;
	QString label_translation() const;

	std::vector<table_col_t> m_columns;
	document_kind_t m_kind = document_kind_t::dict;
};
