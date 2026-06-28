#pragma once

#include "table_row.hpp"

class row_provider_t
{
public:
	virtual ~row_provider_t() = default;
	virtual int row_count() const = 0;
	virtual const table_row_t * row_at(int row) const = 0;
};
