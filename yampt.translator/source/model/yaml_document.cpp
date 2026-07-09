#include "yaml_document.hpp"
#include <io/yaml_l10n_writer.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>

yaml_document_t::yaml_document_t(const std::string & clicked_path, const std::string & native_language_code)
    : m_native_code(native_language_code)
{
	resolve_paths(clicked_path, native_language_code);
	load_foreign();
	load_native();
}

void yaml_document_t::resolve_paths(const std::string & clicked_path, const std::string & native_code)
{
	namespace fs = std::filesystem;

	auto normalized = string_utils::normalize_path(clicked_path);
	auto dir = fs::path(normalized).parent_path();
	auto native_filename = native_code + ".yaml";

	m_native_path = string_utils::normalize_path((dir / native_filename).string());

	auto clicked_stem = fs::path(normalized).stem().string();
	if (clicked_stem == native_code)
	{
		auto en_path = dir / "en.yaml";
		if (fs::exists(en_path))
		{
			m_foreign_path = string_utils::normalize_path(en_path.string());
			return;
		}

		for (const auto & entry : fs::directory_iterator(dir))
		{
			if (!entry.is_regular_file())
				continue;

			if (entry.path().extension() != ".yaml")
				continue;

			auto stem = entry.path().stem().string();
			if (stem == native_code)
				continue;

			m_foreign_path = string_utils::normalize_path(entry.path().string());
			return;
		}

		m_foreign_path = normalized;
	}
	else
	{
		m_foreign_path = normalized;
	}
}

void yaml_document_t::load_foreign()
{
	yaml_l10n_reader_t reader;
	if (!reader.load(m_foreign_path))
		return;

	for (const auto & entry : reader.source_entries())
	{
		m_keys.push_back(entry.key);
		m_foreign_values.push_back(entry.value);
		m_native_values.push_back("");
	}
}

void yaml_document_t::load_native()
{
	namespace fs = std::filesystem;

	if (!fs::exists(m_native_path))
		return;

	yaml_l10n_reader_t reader;
	if (!reader.load(m_native_path))
		return;

	std::unordered_map<std::string, std::string> native_map;
	for (const auto & entry : reader.source_entries())
		native_map[entry.key] = entry.value;

	for (size_t i = 0; i < m_keys.size(); ++i)
	{
		auto it = native_map.find(m_keys[i]);
		if (it == native_map.end())
			continue;

		if (it->second.empty())
			continue;

		m_native_values[i] = it->second;
		m_modified_indices.insert(i);
	}
}

std::string yaml_document_t::path() const
{
	return m_foreign_path;
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
	rows.reserve(m_keys.size());

	for (size_t i = 0; i < m_keys.size(); ++i)
	{
		table_row_t row;
		row.type = rec_type_t::yaml;
		row.key_text = m_keys[i];
		row.old_text = m_foreign_values[i];
		row.new_text = m_modified_indices.count(i) ? m_native_values[i] : "";
		row.status = m_modified_indices.count(i) ? status_t::in_progress : status_t::untranslated;
		row.record_index = i;
		rows.push_back(std::move(row));
	}

	return rows;
}

void yaml_document_t::commit_edit(rec_type_t, size_t record_index, const std::string & new_text)
{
	if (record_index >= m_keys.size())
		return;

	m_native_values[record_index] = new_text;
	m_modified_indices.insert(record_index);
	m_dirty = true;
}

void yaml_document_t::save()
{
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
	writer.write(m_native_path, entries, key_order);
	m_dirty = false;
}

int yaml_document_t::translated_count() const
{
	return static_cast<int>(m_modified_indices.size());
}

int yaml_document_t::total_count() const
{
	return static_cast<int>(m_keys.size());
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

void yaml_document_t::export_to(const std::string & output_path)
{
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
	writer.write(output_path, entries, key_order);
}
