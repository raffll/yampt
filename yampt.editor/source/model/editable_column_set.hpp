#pragma once

#include <set>
#include <vector>

class editable_column_set_t
{
public:
	void clear_all();
	void set_merge_column(int column);
	bool is_editable(int column) const;

	void toggle_plugin_editable(int plugin_index, bool editable);
	bool is_plugin_editable(int plugin_index) const;
	void remove_plugin(int plugin_index);

	void rebuild_for_record(const std::vector<int> & column_plugin_indices, int merge_col);

private:
	std::set<int> m_editable_plugins;
	std::set<int> m_editable_columns;
	int m_merge_column = -1;
};
