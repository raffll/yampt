#pragma once

#include "edit_permissions.hpp"
#include "table_row.hpp"
#include <string>
#include <vector>

enum class document_kind_t
{
	dict,
	yaml,
	plugin
};

struct commit_result_t
{
	std::string new_text;
	status_t status = status_t::untranslated;
	int propagated_count = 0;
	bool success = false;
};

class document_t
{
public:
	virtual ~document_t() = default;

	virtual document_kind_t kind() const = 0;
	virtual std::string path() const = 0;
	virtual bool is_dirty() const = 0;
	virtual bool is_read_only() const = 0;
	virtual document_permissions_t permissions() const = 0;

	virtual std::vector<table_row_t> build_rows() const = 0;
	virtual void commit_edit(rec_type_t type, size_t record_index, const std::string & new_text) = 0;
	virtual commit_result_t commit(const table_row_t & row, const std::string & new_text, status_t intent) = 0;
	virtual commit_result_t commit_status(const table_row_t & row, status_t new_status) = 0;
	virtual commit_result_t reset_to_original(const table_row_t & row) = 0;
	virtual void save() = 0;

	virtual int translated_count() const = 0;
	virtual int total_count() const = 0;

	virtual void set_dirty(bool dirty) = 0;
};
