#include "dict_document.hpp"
#include <io/dict_reader.hpp>
#include <io/dict_writer.hpp>
#include <utility/string_utils.hpp>

dict_document_t::dict_document_t(const std::string & path, codepage_t codepage, dict_kind_t kind)
    : m_path(path)
    , m_codepage(codepage)
    , m_kind(kind)
{
	m_path = string_utils::normalize_path(m_path);

	dict_reader_t reader(path);
	if (!reader.is_loaded())
		return;

	m_data = reader.get_dict();

	for (auto & [type, chapter] : m_data)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = decode_to_utf8(entry.key_text, m_codepage);
			entry.old_text = decode_to_utf8(entry.old_text, m_codepage);
			entry.new_text = decode_to_utf8(entry.new_text, m_codepage);

			if (!entry.details.empty())
				entry.details = decode_to_utf8(entry.details, m_codepage);
		}
	}
}

std::string dict_document_t::path() const
{
	return m_path;
}

bool dict_document_t::is_dirty() const
{
	return m_dirty;
}

bool dict_document_t::is_read_only() const
{
	return false;
}

std::vector<table_row_t> dict_document_t::build_rows() const
{
	std::vector<table_row_t> rows;

	for (const auto & [type, chapter] : m_data)
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
	auto it = m_data.find(type);
	if (it == m_data.end())
		return;

	if (record_index >= it->second.records.size())
		return;

	it->second.records[record_index].new_text = new_text;
	m_dirty = true;
}

void dict_document_t::save()
{
	tools_t::dict_t encoded;

	for (const auto & [type, chapter] : m_data)
	{
		for (const auto & rec : chapter.records)
		{
			tools_t::record_entry_t entry;
			entry.key_text = encode_from_utf8(rec.key_text, m_codepage);
			entry.old_text = encode_from_utf8(rec.old_text, m_codepage);
			entry.new_text = encode_from_utf8(rec.new_text, m_codepage);
			entry.status = rec.status;
			entry.speaker_name = rec.speaker_name;
			entry.gender = rec.gender;
			entry.enchantment = rec.enchantment;

			if (!rec.details.empty())
				entry.details = encode_from_utf8(rec.details, m_codepage);

			encoded[type].insert(entry);
		}
	}

	dict_writer_t::write(encoded, m_path);
	m_dirty = false;
	m_modified_records.clear();
}

int dict_document_t::translated_count() const
{
	int count = 0;
	for (const auto & [type, chapter] : m_data)
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
	for (const auto & [type, chapter] : m_data)
		count += static_cast<int>(chapter.records.size());

	return count;
}

void dict_document_t::set_dirty(bool dirty)
{
	m_dirty = dirty;
}

dict_kind_t dict_document_t::kind() const
{
	return m_kind;
}

const tools_t::dict_t & dict_document_t::data() const
{
	return m_data;
}

tools_t::dict_t & dict_document_t::data_mut()
{
	return m_data;
}

const std::set<std::pair<tools_t::rec_type_t, size_t>> & dict_document_t::modified_records() const
{
	return m_modified_records;
}

void dict_document_t::modified_records_insert(tools_t::rec_type_t type, size_t record_index)
{
	m_modified_records.insert({ type, record_index });
}
