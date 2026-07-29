#pragma once

#include "document.hpp"
#include <io/codepage.hpp>
#include <utility/domain_types.hpp>
#include <set>
#include <string>

class eet_document_t : public document_t
{
public:
	eet_document_t(const std::string & path, codepage_t codepage);

	document_kind_t kind() const override;
	std::string path() const override;
	bool is_dirty() const override;
	bool is_read_only() const override;
	document_permissions_t permissions() const override;

	std::vector<table_row_t> build_rows() const override;
	void commit_edit(rec_type_t type, size_t record_index, const std::string & new_text) override;
	commit_result_t commit(const table_row_t & row, const std::string & new_text, status_t intent) override;
	commit_result_t commit_status(const table_row_t & row, status_t new_status) override;
	commit_result_t reset_to_original(const table_row_t & row) override;
	void save() override;

	int translated_count() const override;
	int total_count() const override;
	std::set<rec_type_t> supported_types() const override;
	std::set<status_t> supported_statuses() const override;
	void set_dirty(bool dirty) override;

	bool export_as_dict(const std::string & output_path) const;
	size_t converted_count() const;
	size_t skipped_count() const;

private:
	std::string m_path;
	dict_t m_dict;
	size_t m_converted_count = 0;
	size_t m_skipped_count = 0;
};
