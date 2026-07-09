#pragma once

#include "document.hpp"
#include <io/yaml_l10n_reader.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class yaml_document_t : public document_t
{
public:
	yaml_document_t(const std::string & clicked_path, const std::string & native_language_code);

	std::string path() const override;
	bool is_dirty() const override;
	bool is_read_only() const override;
	document_permissions_t permissions() const override;

	std::vector<table_row_t> build_rows() const override;
	void commit_edit(rec_type_t type, size_t record_index, const std::string & new_text) override;
	commit_result_t commit(const table_row_t & row, const std::string & new_text, status_t intent) override;
	void save() override;

	int translated_count() const override;
	int total_count() const override;

	void set_dirty(bool dirty) override;

	const std::string & foreign_path() const;
	const std::string & native_path() const;
	bool is_native_file() const;
	void export_native();
	void export_to(const std::string & output_path);

private:
	void load_as_native();
	void load_as_foreign();

	std::string m_path;
	std::string m_foreign_path;
	std::string m_native_path;
	std::string m_clicked_path;
	std::string m_native_code;
	bool m_is_native_file = false;

	std::vector<std::string> m_keys;
	std::vector<std::string> m_foreign_values;
	std::vector<std::string> m_native_values;
	std::set<size_t> m_modified_indices;
	bool m_dirty = false;
};
