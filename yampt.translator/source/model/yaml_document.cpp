#include "yaml_document.hpp"
#include <io/yaml_l10n_writer.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>

yaml_document_t::yaml_document_t(const std::string & file_path)
    : m_path(file_path)
    , m_tmp_path(file_path + ".tmp")
{
	m_path = string_utils::normalize_path(m_path);
	m_tmp_path = m_path + ".tmp";

	yaml_l10n_reader_t reader;
	if (!reader.load(m_path))
		return;

	for (const auto & src : reader.source_entries())
	{
		l10n_entry_t e;
		e.key = src.key;
		e.value = src.value;
		m_source_values.push_back(src.value);
		m_entries.push_back(std::move(e));
	}

	yaml_l10n_reader_t tmp_reader;
	if (!tmp_reader.load(m_tmp_path))
		return;

	std::unordered_map<std::string, std::string> tmp_map;
	for (const auto & e : tmp_reader.source_entries())
		tmp_map[e.key] = e.value;

	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		auto it = tmp_map.find(m_entries[i].key);
		if (it == tmp_map.end())
			continue;

		m_entries[i].value = it->second;
		m_modified_indices.insert(i);
	}
}

std::string yaml_document_t::path() const
{
	return m_path;
}

bool yaml_document_t::is_dirty() const
{
	return m_dirty;
}

bool yaml_document_t::is_read_only() const
{
	return false;
}

std::vector<table_row_t> yaml_document_t::build_rows() const
{
	std::vector<table_row_t> rows;
	rows.reserve(m_entries.size());

	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		table_row_t row;
		row.type = tools_t::rec_type_t::yaml;
		row.key_text = m_entries[i].key;
		row.old_text = (i < m_source_values.size()) ? m_source_values[i] : "";
		row.new_text = m_modified_indices.count(i) ? m_entries[i].value : "";

		if (m_modified_indices.count(i))
			row.status = status_t::in_progress;
		else
			row.status = status_t::untranslated;

		row.record_index = i;
		rows.push_back(std::move(row));
	}

	return rows;
}

void yaml_document_t::commit_edit(tools_t::rec_type_t type, size_t record_index, const std::string & new_text)
{
	if (record_index >= m_entries.size())
		return;

	m_entries[record_index].value = new_text;
	m_modified_indices.insert(record_index);
	m_dirty = true;
}

void yaml_document_t::save()
{
	save_tmp();
	m_dirty = false;
}

int yaml_document_t::translated_count() const
{
	return static_cast<int>(m_modified_indices.size());
}

int yaml_document_t::total_count() const
{
	return static_cast<int>(m_entries.size());
}

void yaml_document_t::set_dirty(bool dirty)
{
	m_dirty = dirty;
}

bool yaml_document_t::has_tmp() const
{
	std::error_code ec;
	return std::filesystem::exists(m_tmp_path, ec);
}

const std::string & yaml_document_t::tmp_path() const
{
	return m_tmp_path;
}

void yaml_document_t::save_tmp()
{
	if (m_modified_indices.empty())
		return;

	std::vector<l10n_entry_t> modified_entries;
	std::vector<std::string> key_order;
	for (auto idx : m_modified_indices)
	{
		if (idx >= m_entries.size())
			continue;

		modified_entries.push_back(m_entries[idx]);
		key_order.push_back(m_entries[idx].key);
	}

	yaml_l10n_writer_t writer;
	writer.write(m_tmp_path, modified_entries, key_order);
}

void yaml_document_t::export_to(const std::string & output_path)
{
	std::vector<std::string> key_order;
	for (const auto & e : m_entries)
		key_order.push_back(e.key);

	yaml_l10n_writer_t writer;
	writer.write(output_path, m_entries, key_order);

	std::error_code ec;
	std::filesystem::remove(m_tmp_path, ec);
	m_modified_indices.clear();
	m_dirty = false;
}
