#include "model/editable_column_set.hpp"

void editable_column_set_t::clear_all()
{
	m_editable_plugins.clear();
	m_editable_columns.clear();
	m_merge_column = -1;
}

void editable_column_set_t::set_merge_column(int column)
{
	m_merge_column = column;
}

bool editable_column_set_t::is_editable(int column) const
{
	return m_editable_columns.contains(column);
}

void editable_column_set_t::toggle_plugin_editable(int plugin_index, bool editable)
{
	if (editable)
		m_editable_plugins.insert(plugin_index);
	else
		m_editable_plugins.erase(plugin_index);
}

bool editable_column_set_t::is_plugin_editable(int plugin_index) const
{
	return m_editable_plugins.contains(plugin_index);
}

void editable_column_set_t::remove_plugin(int plugin_index)
{
	m_editable_plugins.erase(plugin_index);
}

void editable_column_set_t::rebuild_for_record(const std::vector<int> & column_plugin_indices, int merge_col)
{
	m_editable_columns.clear();
	m_merge_column = merge_col;

	if (m_merge_column >= 0)
		m_editable_columns.insert(m_merge_column);

	for (size_t index = 0; index < column_plugin_indices.size(); ++index)
	{
		const int plugin_idx = column_plugin_indices[index];
		if (m_editable_plugins.contains(plugin_idx))
			m_editable_columns.insert(static_cast<int>(index) + 1);
	}
}
