#pragma once

#include "document.hpp"
#include <io/codepage.hpp>
#include <utility/dict_kind.hpp>
#include <utility/tools.hpp>
#include <set>
#include <string>
#include <utility>

class dict_document_t : public document_t
{
public:
	dict_document_t(const std::string & path, codepage_t codepage, dict_kind_t kind);

	std::string path() const override;
	bool is_dirty() const override;
	bool is_read_only() const override;

	std::vector<table_row_t> build_rows() const override;
	void commit_edit(tools_t::rec_type_t type, size_t record_index, const std::string & new_text) override;
	void save() override;

	int translated_count() const override;
	int total_count() const override;

	void set_dirty(bool dirty) override;

	dict_kind_t kind() const;
	const tools_t::dict_t & data() const;
	tools_t::dict_t & data_mut();
	const std::set<std::pair<tools_t::rec_type_t, size_t>> & modified_records() const;
	void modified_records_insert(tools_t::rec_type_t type, size_t record_index);

private:
	std::string path_;
	codepage_t codepage_;
	dict_kind_t kind_;
	tools_t::dict_t data_;
	bool dirty_ = false;
	std::set<std::pair<tools_t::rec_type_t, size_t>> modified_records_;
};
