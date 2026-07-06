#pragma once

#include "table_row.hpp"
#include <string>
#include <vector>

class document_t
{
public:
	virtual ~document_t() = default;

	virtual std::string path() const = 0;
	virtual bool is_dirty() const = 0;
	virtual bool is_read_only() const = 0;

	virtual std::vector<table_row_t> build_rows() const = 0;
	virtual void commit_edit(rec_type_t type, size_t record_index, const std::string & new_text) = 0;
	virtual void save() = 0;

	virtual int translated_count() const = 0;
	virtual int total_count() const = 0;

	virtual void set_dirty(bool dirty) = 0;
};
