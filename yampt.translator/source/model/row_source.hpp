#pragma once

#include "table_row.hpp"

class row_source_t
{
public:
	virtual ~row_source_t() = default;
	virtual int row_count() const = 0;
	virtual const table_row_t * row_at(int row) const = 0;
};
