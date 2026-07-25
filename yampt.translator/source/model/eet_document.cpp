#include "model/eet_document.hpp"
#include <io/dict_writer.hpp>
#include <io/eet_converter.hpp>
#include <io/eet_reader.hpp>

eet_document_t::eet_document_t(const std::string & path)
    : m_path(path)
{
	eet_reader_t reader;
	if (!reader.load(path))
		return;

	eet_converter_t converter(reader.entries());
	m_dict = converter.get_dict();
	m_converted_count = converter.converted_count();
	m_skipped_count = converter.skipped_count();
}

document_kind_t eet_document_t::kind() const
{
	return document_kind_t::eet;
}

std::string eet_document_t::path() const
{
	return m_path;
}

bool eet_document_t::is_dirty() const
{
	return false;
}

bool eet_document_t::is_read_only() const
{
	return true;
}

document_permissions_t eet_document_t::permissions() const
{
	return { false, false, false, false, false };
}

std::vector<table_row_t> eet_document_t::build_rows() const
{
	return {};
}

void eet_document_t::commit_edit(rec_type_t, size_t, const std::string &)
{
}

commit_result_t eet_document_t::commit(const table_row_t &, const std::string &, status_t)
{
	return { .success = false };
}

commit_result_t eet_document_t::commit_status(const table_row_t &, status_t)
{
	return { .success = false };
}

commit_result_t eet_document_t::reset_to_original(const table_row_t &)
{
	return { .success = false };
}

void eet_document_t::save()
{
}

int eet_document_t::translated_count() const
{
	return 0;
}

int eet_document_t::total_count() const
{
	return static_cast<int>(m_converted_count);
}

std::set<rec_type_t> eet_document_t::supported_types() const
{
	return {};
}

std::set<status_t> eet_document_t::supported_statuses() const
{
	return {};
}

void eet_document_t::set_dirty(bool)
{
}

bool eet_document_t::export_as_dict(const std::string & output_path) const
{
	if (m_dict.empty())
		return false;

	dict_writer_t::write(m_dict, output_path);
	return true;
}

size_t eet_document_t::converted_count() const
{
	return m_converted_count;
}

size_t eet_document_t::skipped_count() const
{
	return m_skipped_count;
}
