#include "esm_reader.hpp"
#include "../utility/app_logger.hpp"
#include "../utility/string_utils.hpp"
#include "binary_file_io.hpp"
#include <string_view>

static bool is_plausible_record_tag(std::string_view tag)
{
	if (tag.size() != esm_reader_t::sub_record_id_size)
		return false;

	for (const char character : tag)
	{
		const bool is_upper = character >= 'A' && character <= 'Z';
		const bool is_digit = character >= '0' && character <= '9';
		const bool is_underscore = character == '_';
		if (!is_upper && !is_digit && !is_underscore)
			return false;
	}

	return true;
}

struct record_validation_result_t
{
	bool valid = false;
	size_t record_size = 0;
};

static bool sub_records_tile_exactly(const std::string & record_content)
{
	const auto body_end = record_content.size();
	auto cursor = esm_reader_t::record_header_size;

	while (cursor < body_end)
	{
		if (cursor + esm_reader_t::sub_record_header_size > body_end)
			return false;

		const auto & size_field =
		    record_content.substr(cursor + esm_reader_t::sub_record_id_size, esm_reader_t::record_size_field_length);
		const auto sub_size = domain_types::convert_string_byte_array_to_uint(size_field);
		const auto next_cursor = cursor + esm_reader_t::sub_record_header_size + sub_size;

		if (next_cursor > body_end)
			return false;

		cursor = next_cursor;
	}

	return cursor == body_end;
}

static record_validation_result_t validate_record_at(std::string_view content, size_t record_begin)
{
	const auto & record_tag = std::string(content.substr(record_begin, esm_reader_t::sub_record_id_size));

	if (!is_plausible_record_tag(record_tag))
	{
		const auto & sanitized_tag = string_utils::replace_non_printable_with_dot(record_tag);
		app_logger_t::add_log(
		    "[error] record at offset " + std::to_string(record_begin) + " has an invalid type tag \"" +
		    sanitized_tag + "\", stopping (possibly broken file or record)\r\n");
		return {};
	}

	const auto & size_bytes = std::string(
	    content.substr(record_begin + esm_reader_t::record_size_field_offset, esm_reader_t::record_size_field_length));
	const auto record_size =
	    domain_types::convert_string_byte_array_to_uint(size_bytes) + esm_reader_t::record_header_size;

	app_logger_t::add_log(
	    "[debug] record_begin=" + std::to_string(record_begin) + " size=" + std::to_string(record_size) +
	        " id=" + record_tag + "\r\n",
	    true);

	if (record_begin + record_size > content.size())
	{
		app_logger_t::add_log(
		    "[warning] record at offset " + std::to_string(record_begin) + " declares size " +
		    std::to_string(record_size) + " which exceeds file size, stopping\r\n");
		return {};
	}

	const auto & record_content = std::string(content.substr(record_begin, record_size));

	if (!sub_records_tile_exactly(record_content))
	{
		app_logger_t::add_log(
		    "[error] record at offset " + std::to_string(record_begin) + " (tag \"" + record_tag +
		    "\") declares size " + std::to_string(record_size) +
		    " but its sub-records do not tile exactly to the record body, stopping (possibly broken file "
		    "or record)\r\n");
		return {};
	}

	app_logger_t::add_log(
	    "[debug] tiling ok record_begin=" + std::to_string(record_begin) + " size=" +
	        std::to_string(record_size) + "\r\n",
	    true);

	return { true, record_size };
}

esm_reader_t::esm_reader_t(const std::string & path)
{
	const auto & content = binary_file_io::read_file(path);

	if (!content.empty())
		split_file(content, path);

	m_name.set_name(path);
	set_time(path);
}

void esm_reader_t::split_file(const std::string & content, const std::string & path)
{
	if (content.size() <= sub_record_id_size || content.substr(0, sub_record_id_size) != "TES3")
	{
		app_logger_t::add_log("[error] parsing \"" + path + "\" (not a TES3 plugin)\r\n");
		m_loaded = false;
		return;
	}

	try
	{
		size_t record_begin = 0;
		while (record_begin != content.size())
		{
			const auto & validation = validate_record_at(content, record_begin);

			if (!validation.valid)
				break;

			const auto & record_content = content.substr(record_begin, validation.record_size);
			const auto & record_id = record_content.substr(0, sub_record_id_size);
			m_records.push_back({ record_id, record_content, record_content.size(), false });

			record_begin += validation.record_size;
		}
		m_loaded = true;
	}
	catch (const std::exception & error)
	{
		app_logger_t::add_log("[error] parsing \"" + path + "\" (possibly broken file or record)\r\n");
		app_logger_t::add_log("[error] exception: " + std::string(error.what()) + "\r\n");
		m_loaded = false;
	}
}

void esm_reader_t::set_time(const std::string & path)
{
	if (m_loaded)
		m_time = std::filesystem::last_write_time(path);
}

void esm_reader_t::select_record(size_t index)
{
	if (!m_loaded)
		return;

	ptr_record = &m_records.at(index);
	m_key = {};
	m_value = {};
}

void esm_reader_t::replace_record(const std::string & content)
{
	if (!m_loaded)
		return;

	ptr_record->content = content;
	ptr_record->modified = true;
	ptr_record->size = content.size();
}

void esm_reader_t::remove_record(size_t index)
{
	if (!m_loaded || index >= m_records.size())
		return;

	std::vector<record_t> kept_records;
	kept_records.reserve(m_records.size() - 1);
	for (size_t i = 0; i < m_records.size(); ++i)
	{
		if (i == index)
			continue;

		kept_records.push_back(m_records[i]);
	}

	m_records = std::move(kept_records);

	ptr_record = nullptr;
	m_key = {};
	m_value = {};
}

void esm_reader_t::set_modified(size_t index)
{
	if (m_loaded)
		m_records.at(index).modified = true;
}

void esm_reader_t::set_key(const std::string & sub_id)
{
	if (!m_loaded)
		return;

	m_key.sub_id = sub_id;
	try
	{
		scan_sub_records(record_header_size, m_key);
	}
	catch (const std::exception & error)
	{
		handle_exception(error);
	}
}

void esm_reader_t::set_value(const std::string & sub_id)
{
	if (!m_loaded)
		return;

	m_value.sub_id = sub_id;
	m_value.counter = 0;
	try
	{
		scan_sub_records(record_header_size, m_value);
	}
	catch (const std::exception & error)
	{
		handle_exception(error);
	}
}

void esm_reader_t::set_next_value(const std::string & sub_id)
{
	if (!m_loaded || !m_value.exist)
		return;

	m_value.sub_id = sub_id;
	const auto current_size = domain_types::convert_string_byte_array_to_uint(
	    ptr_record->content.substr(m_value.pos + sub_record_id_size, sub_record_id_size));
	const auto next_pos = m_value.pos + sub_record_header_size + current_size;
	m_value.counter++;

	try
	{
		scan_sub_records(next_pos, m_value);
	}
	catch (const std::exception & error)
	{
		handle_exception(error);
	}
}

void esm_reader_t::scan_sub_records(size_t start_pos, sub_record_t & target)
{
	auto scan_pos = start_pos;
	const auto & record_content = ptr_record->content;
	const auto record_length = record_content.size();

	while (scan_pos < record_length)
	{
		if (scan_pos + sub_record_header_size > record_length)
			break;

		const auto & found_id = record_content.substr(scan_pos, sub_record_id_size);
		const auto found_size = domain_types::convert_string_byte_array_to_uint(
		    record_content.substr(scan_pos + sub_record_id_size, sub_record_id_size));

		if (found_size == 0)
			break;

		if (scan_pos + sub_record_header_size + found_size > record_length)
			break;

		if (found_id == target.sub_id)
		{
			target.content = record_content.substr(scan_pos + sub_record_header_size, found_size);
			target.text = string_utils::erase_null_chars(target.content);
			target.pos = scan_pos;
			target.size = found_size;
			target.exist = true;
			return;
		}

		scan_pos += sub_record_header_size + found_size;
	}

	mark_not_found(target);
}

void esm_reader_t::mark_not_found(sub_record_t & target)
{
	target.content = "N/A";
	target.text = "N/A";
	target.pos = ptr_record->content.size();
	target.size = 0;
	target.exist = false;
}

void esm_reader_t::handle_exception(const std::exception & error)
{
	const auto & sanitized = string_utils::replace_non_printable_with_dot(ptr_record->content);
	app_logger_t::add_log("[error] in function (possibly broken record)\r\n");
	app_logger_t::add_log(sanitized + "\r\n");
	app_logger_t::add_log("[error] exception: " + std::string(error.what()) + "\r\n");
	m_loaded = false;
}

size_t esm_reader_t::get_modified_count()
{
	size_t count = 0;
	for (const auto & record : m_records)
	{
		if (record.modified)
			count++;
	}
	return count;
}
