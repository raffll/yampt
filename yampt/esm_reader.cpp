#include "esm_reader.hpp"

esm_reader_t::esm_reader_t(const std::string & path)
{
	std::string content = tools_t::read_file(path);

	if (!content.empty())
		split_file(content, path);

	name.set_name(path);
	set_time(path);
}

void esm_reader_t::split_file(const std::string & content, const std::string & path)
{
	if (content.size() > 4 && content.substr(0, 4) == "TES3")
	{
		try
		{
			size_t rec_beg = 0;
			size_t rec_size = 0;
			size_t rec_end = 0;
			while (rec_end != content.size())
			{
				rec_beg = rec_end;
				rec_size = tools_t::convert_string_byte_array_to_uint(content.substr(rec_beg + 4, 4)) + 16;
				rec_end = rec_beg + rec_size;

				if (rec_end > content.size())
				{
					tools_t::add_log(
					    "[warning] record at offset " + std::to_string(rec_beg) + " declares size " +
					    std::to_string(rec_size) + " which exceeds file size, stopping\r\n");
					break;
				}

				const auto & cnt = content.substr(rec_beg, rec_size);
				const auto & size = cnt.size();
				const auto & id = cnt.substr(0, 4);
				records.push_back({ id, cnt, size, false });
			}
			loaded_ = true;
		}
		catch (const std::exception & e)
		{
			tools_t::add_log("[error] parsing \"" + path + "\" (possibly broken file or record)\r\n");
			tools_t::add_log("[error] exception: " + std::string(e.what()) + "\r\n");
			loaded_ = false;
		}
	}
	else
	{
		tools_t::add_log("[error] parsing \"" + path + "\" (not a TES3 plugin)\r\n");
		loaded_ = false;
	}
}

void esm_reader_t::set_time(const std::string & path)
{
	if (loaded_)
	{
		time = std::filesystem::last_write_time(path);
	}
}

void esm_reader_t::select_record(size_t i)
{
	if (loaded_)
	{
		rec = &records.at(i);
		key = {};
		value = {};
	}
}

void esm_reader_t::replace_record(const std::string & content)
{
	if (loaded_)
	{
		rec->content = content;
		rec->modified = true;
		rec->size = content.size();
	}
}

void esm_reader_t::set_modified(size_t i)
{
	if (loaded_)
	{
		records.at(i).modified = true;
	}
}

void esm_reader_t::set_key(const std::string & id)
{
	if (loaded_)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		std::string cur_id;
		std::string cur_text;
		key.id = id;

		try
		{
			main_loop(cur_pos, cur_size, cur_id, cur_text, key);
		}
		catch (const std::exception & e)
		{
			handle_exception(e);
		}
	}
}

void esm_reader_t::set_value(const std::string & id)
{
	if (loaded_)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		std::string cur_id;
		std::string cur_text;
		value.id = id;
		value.counter = 0;

		try
		{
			main_loop(cur_pos, cur_size, cur_id, cur_text, value);
		}
		catch (const std::exception & e)
		{
			handle_exception(e);
		}
	}
}

void esm_reader_t::set_next_value(const std::string & id)
{
	if (loaded_ && value.exist)
	{
		size_t cur_pos;
		size_t cur_size;
		std::string cur_id;
		std::string cur_text;
		value.id = id;

		cur_pos = value.pos;
		cur_size = tools_t::convert_string_byte_array_to_uint(rec->content.substr(cur_pos + 4, 4));
		cur_pos += 8 + cur_size;
		value.counter++;

		try
		{
			main_loop(cur_pos, cur_size, cur_id, cur_text, value);
		}
		catch (const std::exception & e)
		{
			handle_exception(e);
		}
	}
}

void esm_reader_t::main_loop(
    std::size_t & cur_pos,
    std::size_t & cur_size,
    std::string & cur_id,
    std::string & cur_text,
    esm_reader_t::sub_record_t & subrecord)
{
	while (cur_pos != rec->content.size())
	{
		if (cur_pos + 8 > rec->content.size())
			break;
		cur_id = rec->content.substr(cur_pos, 4);
		cur_size = tools_t::convert_string_byte_array_to_uint(rec->content.substr(cur_pos + 4, 4));
		if (cur_size == 0)
			break;
		if (cur_pos + 8 + cur_size > rec->content.size())
			break;
		if (cur_id == subrecord.id)
		{
			cur_text = rec->content.substr(cur_pos + 8, cur_size);
			subrecord.content = cur_text;
			subrecord.text = tools_t::erase_null_chars(subrecord.content);
			subrecord.pos = cur_pos;
			subrecord.size = cur_size;
			subrecord.exist = true;
			break;
		}
		cur_pos += 8 + cur_size;
	}

	if (cur_pos == rec->content.size())
	{
		subrecord.content = "N/A";
		subrecord.text = "N/A";
		subrecord.pos = cur_pos;
		subrecord.size = 0;
		subrecord.exist = false;
	}
}

void esm_reader_t::handle_exception(const std::exception & e)
{
	std::string cur_rec = tools_t::replace_non_readable_chars_with_dot(rec->content);
	tools_t::add_log("[error] in function (possibly broken record)\r\n");
	tools_t::add_log(cur_rec + "\r\n");
	tools_t::add_log("[error] exception: " + std::string(e.what()) + "\r\n");
	loaded_ = false;
}

size_t esm_reader_t::get_modified_count()
{
	size_t count = 0;
	for (const auto & record : records)
	{
		if (record.modified)
			count++;
	}
	return count;
}
