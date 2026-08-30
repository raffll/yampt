#pragma once

class editable_column_set_t
{
public:
	void set_editing_enabled(bool enabled);
	bool is_editing_enabled() const;

	void set_merge_column(int column);

	bool is_editable(int column) const;

private:
	bool m_editing_enabled = false;
	int m_merge_column = -1;
};
