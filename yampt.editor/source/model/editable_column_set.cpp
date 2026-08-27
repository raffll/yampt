#include "model/editable_column_set.hpp"

void editable_column_set_t::set_editing_enabled(bool enabled)
{
	m_editing_enabled = enabled;
}

bool editable_column_set_t::is_editing_enabled() const
{
	return m_editing_enabled;
}

void editable_column_set_t::set_merge_column(int column)
{
	m_merge_column = column;
}

bool editable_column_set_t::is_editable(int column) const
{
	if (column < 1)
		return false;

	if (column == m_merge_column)
		return true;

	return m_editing_enabled;
}
