#include "table_columns.hpp"
#include <algorithm>
#include <QCoreApplication>

table_columns_t::table_columns_t()
    : m_columns(columns_for_kind(document_kind_t::dict))
    , m_kind(document_kind_t::dict)
{}

table_columns_t::table_columns_t(document_kind_t kind)
    : m_columns(columns_for_kind(kind))
    , m_kind(kind)
{}

void table_columns_t::set_for_kind(document_kind_t kind)
{
	m_columns = columns_for_kind(kind);
	m_kind = kind;
}

int table_columns_t::count() const
{
	return static_cast<int>(m_columns.size());
}

table_col_t table_columns_t::at(int visual_position) const
{
	if (visual_position < 0 || visual_position >= static_cast<int>(m_columns.size()))
		return col_count;

	return m_columns[visual_position];
}

int table_columns_t::position_of(table_col_t logical_column) const
{
	const auto it = std::find(m_columns.begin(), m_columns.end(), logical_column);
	if (it == m_columns.end())
		return -1;

	return static_cast<int>(std::distance(m_columns.begin(), it));
}

bool table_columns_t::contains(table_col_t logical_column) const
{
	return position_of(logical_column) >= 0;
}

std::vector<table_col_t> table_columns_t::columns_for_kind(document_kind_t kind)
{
	if (kind == document_kind_t::loc_cel || kind == document_kind_t::loc_top || kind == document_kind_t::loc_mrk)
		return { col_original, col_translation };

	if (kind == document_kind_t::yaml)
		return { col_original, col_translation, col_status };

	return { col_id, col_key, col_original, col_translation, col_status };
}

QString table_columns_t::label_for(table_col_t logical_column) const
{
	const auto translate = [](const char * text) { return QCoreApplication::translate("yTranslator", text); };

	switch (logical_column)
	{
	case col_id:
		return translate("ID");
	case col_key:
		return translate("Key");
	case col_status:
		return translate("Status");
	case col_original:
		return label_original();
	case col_translation:
		return label_translation();
	default:
		return {};
	}
}

QString table_columns_t::label_original() const
{
	const auto translate = [](const char * text) { return QCoreApplication::translate("yTranslator", text); };

	switch (m_kind)
	{
	case document_kind_t::loc_cel:
		return translate("Cell");
	case document_kind_t::loc_top:
		return translate("Topic Form");
	case document_kind_t::loc_mrk:
		return translate("Marker");
	default:
		return translate("Original");
	}
}

QString table_columns_t::label_translation() const
{
	const auto translate = [](const char * text) { return QCoreApplication::translate("yTranslator", text); };

	switch (m_kind)
	{
	case document_kind_t::loc_top:
		return translate("Topic");
	default:
		return translate("Translation");
	}
}
