#pragma once

#include "document.hpp"
#include <io/codepage.hpp>
#include <io/loc_file_reader.hpp>
#include <set>
#include <string>
#include <vector>

class loc_document_t : public document_t
{
public:
	loc_document_t(const std::string & path, codepage_t codepage);

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

	loc_types::loc_file_kind_t file_kind() const;
	void reload();

private:
	rec_type_t record_type_for_file_kind() const;

	std::string m_path;
	codepage_t m_codepage;
	loc_file_reader::loc_file_t m_file;
};
