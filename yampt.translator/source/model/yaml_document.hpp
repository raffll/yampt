#pragma once

#include "../io/yaml_l10n_reader.hpp"
#include "document.hpp"
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class yaml_document_t : public document_t
{
public:
	explicit yaml_document_t(const std::string & file_path);

	std::string path() const override;
	bool is_dirty() const override;
	bool is_read_only() const override;

	std::vector<table_row_t> build_rows() const override;
	void commit_edit(tools_t::rec_type_t type, size_t record_index, const std::string & new_text) override;
	void save() override;

	int translated_count() const override;
	int total_count() const override;

	void set_dirty(bool dirty) override;

	bool has_tmp() const;
	const std::string & tmp_path() const;
	void save_tmp();
	void export_to(const std::string & output_path);

private:
	std::string m_path;
	std::string m_tmp_path;
	std::vector<l10n_entry_t> m_entries;
	std::vector<std::string> m_source_values;
	std::set<size_t> m_modified_indices;
	bool m_dirty = false;
};
