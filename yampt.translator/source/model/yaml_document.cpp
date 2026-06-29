#include "yaml_document.hpp"
#include "../io/yaml_l10n_writer.hpp"
#include <utility/string_utils.hpp>
#include <filesystem>

yaml_document_t::yaml_document_t(const std::string & file_path)
    : path_(file_path)
    , tmp_path_(file_path + ".tmp")
{
	path_ = string_utils::normalize_path(path_);
	tmp_path_ = path_ + ".tmp";

	yaml_l10n_reader_t reader;
	if (!reader.load(path_))
		return;

	for (const auto & src : reader.source_entries())
	{
		l10n_entry_t e;
		e.key = src.key;
		e.value = src.value;
		source_values_.push_back(src.value);
		entries_.push_back(std::move(e));
	}

	yaml_l10n_reader_t tmp_reader;
	if (!tmp_reader.load(tmp_path_))
		return;

	std::unordered_map<std::string, std::string> tmp_map;
	for (const auto & e : tmp_reader.source_entries())
		tmp_map[e.key] = e.value;

	for (size_t i = 0; i < entries_.size(); ++i)
	{
		auto it = tmp_map.find(entries_[i].key);
		if (it == tmp_map.end())
			continue;

		entries_[i].value = it->second;
		modified_indices_.insert(i);
	}
}

std::string yaml_document_t::path() const
{
	return path_;
}

bool yaml_document_t::is_dirty() const
{
	return dirty_;
}

bool yaml_document_t::is_read_only() const
{
	return false;
}

std::vector<table_row_t> yaml_document_t::build_rows() const
{
	std::vector<table_row_t> rows;
	rows.reserve(entries_.size());

	for (size_t i = 0; i < entries_.size(); ++i)
	{
		table_row_t row;
		row.type = tools_t::rec_type_t::yaml;
		row.key_text = entries_[i].key;
		row.old_text = (i < source_values_.size()) ? source_values_[i] : "";
		row.new_text = modified_indices_.count(i) ? entries_[i].value : "";

		if (modified_indices_.count(i))
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
	if (record_index >= entries_.size())
		return;

	entries_[record_index].value = new_text;
	modified_indices_.insert(record_index);
	dirty_ = true;
}

void yaml_document_t::save()
{
	save_tmp();
	dirty_ = false;
}

int yaml_document_t::translated_count() const
{
	return static_cast<int>(modified_indices_.size());
}

int yaml_document_t::total_count() const
{
	return static_cast<int>(entries_.size());
}

void yaml_document_t::set_dirty(bool dirty)
{
	dirty_ = dirty;
}

bool yaml_document_t::has_tmp() const
{
	std::error_code ec;
	return std::filesystem::exists(tmp_path_, ec);
}

const std::string & yaml_document_t::tmp_path() const
{
	return tmp_path_;
}

void yaml_document_t::save_tmp()
{
	if (modified_indices_.empty())
		return;

	std::vector<l10n_entry_t> modified_entries;
	std::vector<std::string> key_order;
	for (auto idx : modified_indices_)
	{
		if (idx >= entries_.size())
			continue;

		modified_entries.push_back(entries_[idx]);
		key_order.push_back(entries_[idx].key);
	}

	yaml_l10n_writer_t writer;
	writer.write(tmp_path_, modified_entries, key_order);
}

void yaml_document_t::export_to(const std::string & output_path)
{
	std::vector<std::string> key_order;
	for (const auto & e : entries_)
		key_order.push_back(e.key);

	yaml_l10n_writer_t writer;
	writer.write(output_path, entries_, key_order);

	std::error_code ec;
	std::filesystem::remove(tmp_path_, ec);
	modified_indices_.clear();
	dirty_ = false;
}
