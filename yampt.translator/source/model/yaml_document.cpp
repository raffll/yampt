#include "yaml_document.hpp"
#include <io/yaml_l10n_writer.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>
#include <fstream>

yaml_document_t::yaml_document_t(const std::string & clicked_path, const std::string & native_language_code)
    : m_native_code(native_language_code)
{
	namespace fs = std::filesystem;

	auto normalized = string_utils::normalize_path(clicked_path);
	auto dir = fs::path(normalized).parent_path();
	auto stem = fs::path(normalized).stem().string();

	m_is_native_file = (stem == native_language_code);
	m_path = normalized;
	m_foreign_path = "";
	m_native_path = string_utils::normalize_path((dir / (native_language_code + ".yaml")).string());

	if (m_is_native_file)
	{
		auto en_path = dir / "en.yaml";
		if (fs::exists(en_path))
			m_foreign_path = string_utils::normalize_path(en_path.string());
		else
		{
			for (const auto & entry : fs::directory_iterator(dir))
			{
				if (!entry.is_regular_file() || entry.path().extension() != ".yaml")
					continue;

				if (entry.path().stem().string() == native_language_code)
					continue;

				m_foreign_path = string_utils::normalize_path(entry.path().string());
				break;
			}
		}

		load_as_native();
	}
	else
	{
		load_as_foreign();
	}
}

void yaml_document_t::load_as_native()
{
	if (!m_foreign_path.empty())
	{
		yaml_l10n_reader_t reader;
		if (reader.load(m_foreign_path))
		{
			for (const auto & entry : reader.source_entries())
			{
				m_keys.push_back(entry.key);
				m_foreign_values.push_back(entry.value);
				m_native_values.push_back("");
			}
		}
	}

	yaml_l10n_reader_t native_reader;
	if (!native_reader.load(m_path))
		return;

	std::unordered_map<std::string, std::string> native_map;
	for (const auto & entry : native_reader.source_entries())
		native_map[entry.key] = entry.value;

	for (size_t i = 0; i < m_keys.size(); ++i)
	{
		auto it = native_map.find(m_keys[i]);
		if (it == native_map.end() || it->second.empty())
			continue;

		m_native_values[i] = it->second;
		m_modified_indices.insert(i);
	}
}

void yaml_document_t::load_as_foreign()
{
	yaml_l10n_reader_t reader;
	if (!reader.load(m_path))
		return;

	for (const auto & entry : reader.source_entries())
	{
		m_keys.push_back(entry.key);
		m_foreign_values.push_back(entry.value);
		m_native_values.push_back(entry.value);
	}
}

std::string yaml_document_t::path() const
{
	return m_path;
}

document_kind_t yaml_document_t::kind() const
{
	return document_kind_t::yaml;
}

bool yaml_document_t::is_dirty() const
{
	return m_dirty;
}

bool yaml_document_t::is_read_only() const
{
	return !m_is_native_file;
}

document_permissions_t yaml_document_t::permissions() const
{
	if (m_is_native_file)
		return { true, true, false, true, false };

	return { false, false, true, false, false };
}

std::vector<table_row_t> yaml_document_t::build_rows() const
{
	std::vector<table_row_t> rows;
	rows.reserve(m_keys.size());

	for (size_t i = 0; i < m_keys.size(); ++i)
	{
		table_row_t row;
		row.type = rec_type_t::yaml;
		row.key_text = m_keys[i];
		row.old_text = m_foreign_values[i];

		if (m_is_native_file)
		{
			row.new_text = m_modified_indices.count(i) ? m_native_values[i] : "";
			row.status = m_modified_indices.count(i) ? status_t::in_progress : status_t::untranslated;
		}
		else
		{
			row.new_text = m_native_values[i];
			row.status = status_t::translated;
		}

		row.record_index = i;
		rows.push_back(std::move(row));
	}

	return rows;
}

void yaml_document_t::commit_edit(rec_type_t, size_t record_index, const std::string & new_text)
{
	if (!m_is_native_file)
		return;

	if (record_index >= m_keys.size())
		return;

	m_native_values[record_index] = new_text;
	m_modified_indices.insert(record_index);
	m_dirty = true;
}

commit_result_t yaml_document_t::commit(const table_row_t & row, const std::string & new_text, status_t intent)
{
	commit_result_t result;

	if (!m_is_native_file)
		return result;

	if (row.record_index >= m_keys.size())
		return result;

	m_native_values[row.record_index] = new_text;
	m_modified_indices.insert(row.record_index);
	m_dirty = true;

	result.new_text = new_text;
	result.status = intent;
	result.success = true;
	return result;
}

commit_result_t yaml_document_t::commit_status(const table_row_t & row, status_t new_status)
{
	return commit(row, row.new_text.empty() ? m_native_values[row.record_index] : row.new_text, new_status);
}

commit_result_t yaml_document_t::reset_to_original(const table_row_t & row)
{
	commit_result_t result;

	if (!m_is_native_file)
		return result;

	if (row.record_index >= m_keys.size())
		return result;

	m_native_values[row.record_index] = "";
	m_modified_indices.erase(row.record_index);
	m_dirty = true;

	result.new_text = m_foreign_values[row.record_index];
	result.status = status_t::untranslated;
	result.success = true;
	return result;
}

void yaml_document_t::save()
{
	if (!m_is_native_file)
		return;

	if (m_modified_indices.empty())
		return;

	std::vector<l10n_entry_t> entries;
	std::vector<std::string> key_order;

	for (size_t i = 0; i < m_keys.size(); ++i)
	{
		if (!m_modified_indices.count(i))
			continue;

		if (m_native_values[i].empty())
			continue;

		l10n_entry_t entry;
		entry.key = m_keys[i];
		entry.value = m_native_values[i];
		entries.push_back(std::move(entry));
		key_order.push_back(m_keys[i]);
	}

	yaml_l10n_writer_t writer;
	writer.write(m_path, entries, key_order);
	m_dirty = false;
}

int yaml_document_t::translated_count() const
{
	if (!m_is_native_file)
		return static_cast<int>(m_keys.size());

	return static_cast<int>(m_modified_indices.size());
}

int yaml_document_t::total_count() const
{
	return static_cast<int>(m_keys.size());
}

std::set<rec_type_t> yaml_document_t::supported_types() const
{
	return { rec_type_t::yaml };
}

std::set<status_t> yaml_document_t::supported_statuses() const
{
	return { status_t::translated, status_t::untranslated };
}

void yaml_document_t::set_dirty(bool dirty)
{
	m_dirty = dirty;
}

const std::string & yaml_document_t::foreign_path() const
{
	return m_foreign_path;
}

const std::string & yaml_document_t::native_path() const
{
	return m_native_path;
}

bool yaml_document_t::is_native_file() const
{
	return m_is_native_file;
}

void yaml_document_t::export_native()
{
	if (m_is_native_file)
		return;

	std::vector<l10n_entry_t> entries;
	std::vector<std::string> key_order;

	for (size_t i = 0; i < m_keys.size(); ++i)
	{
		l10n_entry_t entry;
		entry.key = m_keys[i];
		entry.value = "";
		entries.push_back(std::move(entry));
		key_order.push_back(m_keys[i]);
	}

	yaml_l10n_writer_t writer;
	writer.write(m_native_path, entries, key_order);
}

void yaml_document_t::export_to(const std::string & output_path)
{
	std::vector<l10n_entry_t> entries;
	std::vector<std::string> key_order;

	for (size_t i = 0; i < m_keys.size(); ++i)
	{
		const auto & value = m_modified_indices.count(i) ? m_native_values[i] : m_foreign_values[i];
		if (value.empty())
			continue;

		l10n_entry_t entry;
		entry.key = m_keys[i];
		entry.value = value;
		entries.push_back(std::move(entry));
		key_order.push_back(m_keys[i]);
	}

	yaml_l10n_writer_t writer;
	writer.write(output_path, entries, key_order);
	m_dirty = false;
}
