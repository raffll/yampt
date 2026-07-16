#pragma once

#include "document.hpp"
#include <io/codepage.hpp>
#include <utility/dict_kind.hpp>
#include <utility/domain_types.hpp>
#include <set>
#include <string>
#include <utility>

class dict_document_t : public document_t
{
public:
	dict_document_t(const std::string & path, codepage_t codepage, dict_kind_t kind);

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

	dict_kind_t dict_kind() const;
	const dict_t & data() const;
	dict_t & data_mut();
	const std::set<std::pair<rec_type_t, size_t>> & modified_records() const;
	void modified_records_insert(rec_type_t type, size_t record_index);

private:
	int propagate(rec_type_t source_type, const std::string & old_text, const std::string & new_text);

	std::string m_path;
	codepage_t m_codepage;
	dict_kind_t m_kind;
	dict_t m_data;
	bool m_dirty = false;
	std::set<std::pair<rec_type_t, size_t>> m_modified_records;
};
