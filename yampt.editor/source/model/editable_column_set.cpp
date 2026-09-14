#include "model/editable_column_set.hpp"

void editable_column_set_t::set_merge_column(int column)
{
	m_merge_column = column;
}

bool editable_column_set_t::is_editable(int column) const
{
	if (column < 1)
		return false;

	return column == m_merge_column;
}
