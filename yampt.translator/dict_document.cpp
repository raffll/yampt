#include "dict_document.hpp"
#include "../yampt/dict_reader.hpp"
#include "../yampt/dict_writer.hpp"

#include <algorithm>

dict_document_t::dict_document_t(const std::string & path, codepage_t codepage, dict_kind_t kind)
    : path_(path)
    , codepage_(codepage)
    , kind_(kind)
{
	std::replace(path_.begin(), path_.end(), '\\', '/');

	dict_reader_t reader(path);
	if (!reader.is_loaded())
		return;

	data_ = reader.get_dict();

	for (auto & [type, chapter] : data_)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = decode_to_utf8(entry.key_text, codepage_);
			entry.old_text = decode_to_utf8(entry.old_text, codepage_);
			entry.new_text = decode_to_utf8(entry.new_text, codepage_);

			if (!entry.adapted_from.empty())
				entry.adapted_from = decode_to_utf8(entry.adapted_from, codepage_);
		}
	}
}

std::string dict_document_t::path() const
{
	return path_;
}

bool dict_document_t::is_dirty() const
{
	return dirty_;
}

bool dict_document_t::is_read_only() const
{
	return kind_ == dict_kind_t::base;
}

std::vector<table_row_t> dict_document_t::build_rows() const
{
	std::vector<table_row_t> rows;

	for (const auto & [type, chapter] : data_)
	{
		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			const auto & rec = chapter.records[i];
			table_row_t row;
			row.type = type;
			row.key_text = rec.key_text;
			row.old_text = rec.old_text;
			row.new_text = rec.new_text;
			row.status = rec.status;
			row.record_index = i;
			rows.push_back(std::move(row));
		}
	}

	return rows;
}

void dict_document_t::commit_edit(tools_t::rec_type_t type, size_t record_index, const std::string & new_text)
{
	if (kind_ == dict_kind_t::base)
		return;

	auto it = data_.find(type);
	if (it == data_.end())
		return;

	if (record_index >= it->second.records.size())
		return;

	it->second.records[record_index].new_text = new_text;
	dirty_ = true;
}

void dict_document_t::save()
{
	tools_t::dict_t encoded;

	for (const auto & [type, chapter] : data_)
	{
		for (const auto & rec : chapter.records)
		{
			tools_t::record_entry_t entry;
			entry.key_text = encode_from_utf8(rec.key_text, codepage_);
			entry.old_text = encode_from_utf8(rec.old_text, codepage_);
			entry.new_text = encode_from_utf8(rec.new_text, codepage_);
			entry.status = rec.status;
			entry.speaker_name = rec.speaker_name;
			entry.gender = rec.gender;
			entry.enchantment = rec.enchantment;

			if (!rec.adapted_from.empty())
				entry.adapted_from = encode_from_utf8(rec.adapted_from, codepage_);

			encoded[type].insert(entry);
		}
	}

	dict_writer_t::write(encoded, path_);
	dirty_ = false;
	modified_records_.clear();
}

int dict_document_t::translated_count() const
{
	int count = 0;
	for (const auto & [type, chapter] : data_)
	{
		for (const auto & rec : chapter.records)
		{
			if (!rec.new_text.empty())
				++count;
		}
	}

	return count;
}

int dict_document_t::total_count() const
{
	int count = 0;
	for (const auto & [type, chapter] : data_)
		count += static_cast<int>(chapter.records.size());

	return count;
}

void dict_document_t::set_dirty(bool dirty)
{
	dirty_ = dirty;
}

dict_kind_t dict_document_t::kind() const
{
	return kind_;
}

const tools_t::dict_t & dict_document_t::data() const
{
	return data_;
}

tools_t::dict_t & dict_document_t::data_mut()
{
	return data_;
}

const std::set<std::pair<tools_t::rec_type_t, size_t>> & dict_document_t::modified_records() const
{
	return modified_records_;
}

void dict_document_t::modified_records_insert(tools_t::rec_type_t type, size_t record_index)
{
	modified_records_.insert({ type, record_index });
}
