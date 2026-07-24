#include "model/loc_document.hpp"

loc_document_t::loc_document_t(const std::string & path, codepage_t codepage)
	: m_path(path)
	, m_codepage(codepage)
	, m_file(loc_file_reader::read(path, codepage))
{
}

document_kind_t loc_document_t::kind() const
{
	return document_kind_t::loc;
}

std::string loc_document_t::path() const
{
	return m_path;
}

bool loc_document_t::is_dirty() const
{
	return false;
}

bool loc_document_t::is_read_only() const
{
	return true;
}

document_permissions_t loc_document_t::permissions() const
{
	return { false, false, false, false, false };
}

std::vector<table_row_t> loc_document_t::build_rows() const
{
	const auto type = record_type_for_file_kind();
	std::vector<table_row_t> rows;
	rows.reserve(m_file.entries.size());

	for (size_t index = 0; index < m_file.entries.size(); ++index)
	{
		const auto & entry = m_file.entries[index];
		table_row_t row;
		row.type = type;
		row.key_text = entry.key;
		row.old_text = entry.key;
		row.new_text = entry.value;
		row.status = status_t::translated;
		row.record_index = index;
		rows.push_back(std::move(row));
	}

	return rows;
}

void loc_document_t::commit_edit(rec_type_t, size_t, const std::string &)
{
}

commit_result_t loc_document_t::commit(const table_row_t &, const std::string &, status_t)
{
	return { .success = false };
}

commit_result_t loc_document_t::commit_status(const table_row_t &, status_t)
{
	return { .success = false };
}

commit_result_t loc_document_t::reset_to_original(const table_row_t &)
{
	return { .success = false };
}

void loc_document_t::save()
{
}

int loc_document_t::translated_count() const
{
	return total_count();
}

int loc_document_t::total_count() const
{
	return static_cast<int>(m_file.entries.size());
}

std::set<rec_type_t> loc_document_t::supported_types() const
{
	return { record_type_for_file_kind() };
}

std::set<status_t> loc_document_t::supported_statuses() const
{
	return { status_t::translated };
}

void loc_document_t::set_dirty(bool)
{
}

loc_types::loc_file_kind_t loc_document_t::file_kind() const
{
	return m_file.file_kind;
}

void loc_document_t::reload()
{
	m_file = loc_file_reader::read(m_path, m_codepage);
}

rec_type_t loc_document_t::record_type_for_file_kind() const
{
	switch (m_file.file_kind)
	{
	case loc_types::loc_file_kind_t::cel:
		return rec_type_t::cell;

	case loc_types::loc_file_kind_t::top:
	case loc_types::loc_file_kind_t::mrk:
		return rec_type_t::dial;
	}

	return rec_type_t::unknown;
}
