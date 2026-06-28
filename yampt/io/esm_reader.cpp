#include "esm_reader.hpp"

esm_reader_t::esm_reader_t(const std::string & path)
{
	const auto & content = tools_t::read_file(path);

	if (!content.empty())
		split_file(content, path);

	name_.set_name(path);
	set_time(path);
}

void esm_reader_t::split_file(const std::string & content, const std::string & path)
{
	if (content.size() <= sub_record_id_size || content.substr(0, sub_record_id_size) != "TES3")
	{
		tools_t::add_log("[error] parsing \"" + path + "\" (not a TES3 plugin)\r\n");
		loaded_ = false;
		return;
	}

	try
	{
		size_t record_begin = 0;
		size_t record_end = 0;
		while (record_end != content.size())
		{
			record_begin = record_end;
			const auto & size_bytes = content.substr(record_begin + record_size_field_offset, record_size_field_length);
			const auto record_size = tools_t::convert_string_byte_array_to_uint(size_bytes) + record_header_size;
			record_end = record_begin + record_size;

			if (record_end > content.size())
			{
				tools_t::add_log(
				    "[warning] record at offset " + std::to_string(record_begin) + " declares size " +
				    std::to_string(record_size) + " which exceeds file size, stopping\r\n");
				break;
			}

			const auto & record_content = content.substr(record_begin, record_size);
			const auto & record_id = record_content.substr(0, sub_record_id_size);
			records_.push_back({ record_id, record_content, record_content.size(), false });
		}
		loaded_ = true;
	}
	catch (const std::exception & error)
	{
		tools_t::add_log("[error] parsing \"" + path + "\" (possibly broken file or record)\r\n");
		tools_t::add_log("[error] exception: " + std::string(error.what()) + "\r\n");
		loaded_ = false;
	}
}

void esm_reader_t::set_time(const std::string & path)
{
	if (loaded_)
		time_ = std::filesystem::last_write_time(path);
}

void esm_reader_t::select_record(size_t index)
{
	if (!loaded_)
		return;

	ptr_record = &records_.at(index);
	key_ = {};
	value_ = {};
}

void esm_reader_t::replace_record(const std::string & content)
{
	if (!loaded_)
		return;

	ptr_record->content = content;
	ptr_record->modified = true;
	ptr_record->size = content.size();
}

void esm_reader_t::set_modified(size_t index)
{
	if (loaded_)
		records_.at(index).modified = true;
}

void esm_reader_t::set_key(const std::string & sub_id)
{
	if (!loaded_)
		return;

	key_.sub_id = sub_id;
	try
	{
		scan_sub_records(record_header_size, key_);
	}
	catch (const std::exception & error)
	{
		handle_exception(error);
	}
}

void esm_reader_t::set_value(const std::string & sub_id)
{
	if (!loaded_)
		return;

	value_.sub_id = sub_id;
	value_.counter = 0;
	try
	{
		scan_sub_records(record_header_size, value_);
	}
	catch (const std::exception & error)
	{
		handle_exception(error);
	}
}

void esm_reader_t::set_next_value(const std::string & sub_id)
{
	if (!loaded_ || !value_.exist)
		return;

	value_.sub_id = sub_id;
	const auto current_size = tools_t::convert_string_byte_array_to_uint(
	    ptr_record->content.substr(value_.pos + sub_record_id_size, sub_record_id_size));
	const auto next_pos = value_.pos + sub_record_header_size + current_size;
	value_.counter++;

	try
	{
		scan_sub_records(next_pos, value_);
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
		const auto found_size = tools_t::convert_string_byte_array_to_uint(
		    record_content.substr(scan_pos + sub_record_id_size, sub_record_id_size));

		if (found_size == 0)
			break;

		if (scan_pos + sub_record_header_size + found_size > record_length)
			break;

		if (found_id == target.sub_id)
		{
			target.content = record_content.substr(scan_pos + sub_record_header_size, found_size);
			target.text = tools_t::erase_null_chars(target.content);
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
	const auto & sanitized = tools_t::replace_non_readable_chars_with_dot(ptr_record->content);
	tools_t::add_log("[error] in function (possibly broken record)\r\n");
	tools_t::add_log(sanitized + "\r\n");
	tools_t::add_log("[error] exception: " + std::string(error.what()) + "\r\n");
	loaded_ = false;
}

size_t esm_reader_t::get_modified_count()
{
	size_t count = 0;
	for (const auto & record : records_)
	{
		if (record.modified)
			count++;
	}
	return count;
}
