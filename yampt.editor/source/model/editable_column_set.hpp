#pragma once

class editable_column_set_t
{
public:
	void set_merge_column(int column);

	bool is_editable(int column) const;

private:
	int m_merge_column = -1;
};
