#include "dict_reader.hpp"
#include "../utility/app_logger.hpp"
#include "binary_file_io.hpp"
#include "json_reader.hpp"

dict_reader_t::dict_reader_t(const std::string & path)
{
	dict = domain_types::initialize_dict();

	std::string content = binary_file_io::read_file(path);
	if (!content.empty())
	{
		parse_json(content, path);
		name.set_name(path);
	}
}

void dict_reader_t::parse_record(yyjson_val * record, rec_type_t type, const std::string & type_str)
{
	std::string key_text = json_reader_t::get_string(record, "key", "");
	if (key_text.empty())
	{
		app_logger_t::add_log("[warning] record missing \"key\" field in chapter " + type_str + ", skipping\r\n");
		return;
	}

	record_entry_t entry;
	entry.key_text = std::move(key_text);
	entry.old_text = json_reader_t::get_string(record, "old", "");
	entry.new_text = json_reader_t::get_string(record, "new", "");
	entry.status = string_to_status(json_reader_t::get_string(record, "status", ""));

	if (entry.old_text.empty() && type != rec_type_t::gmst)
		app_logger_t::add_log("[warning] empty \"old\" field in " + type_str + ": " + entry.key_text + "\r\n", true);

	if (entry.new_text.empty() && type != rec_type_t::gmst && type != rec_type_t::script)
		app_logger_t::add_log("[warning] empty \"new\" field in " + type_str + ": " + entry.key_text + "\r\n", true);

	if (type == rec_type_t::info)
	{
		entry.speaker_name = json_reader_t::get_string(record, "speaker_name", "");
		entry.gender = json_reader_t::get_string(record, "gender", "");
	}

	if (type == rec_type_t::fnam)
	{
		entry.enchantment = json_reader_t::get_string(record, "enchantment", "");
	}

	entry.details = json_reader_t::get_string(record, "details", "");

	validate_entry(entry, type);
}

void dict_reader_t::parse_chapter(yyjson_val * array, rec_type_t type, const std::string & type_str)
{
	json_reader_t::foreach_arr(array, [&](size_t, yyjson_val * record) { parse_record(record, type, type_str); });
}

void dict_reader_t::parse_json(const std::string & content, const std::string & path)
{
	json_reader_t reader;
	if (!reader.parse(content.data(), content.size(), true))
	{
		app_logger_t::add_log("[error] parsing \"" + path + "\"\r\n");
		app_logger_t::add_log("[error] " + reader.get_error() + "\r\n");
		m_loaded = false;
		return;
	}

	yyjson_val * root = reader.root();

	json_reader_t::foreach_obj(
	    root,
	    [&](const char * key_str, size_t key_len, yyjson_val * arr)
	{
		std::string type_str(key_str, key_len);

		if (type_str == "encoding")
			return;

		rec_type_t type = domain_types::str_to_type(type_str);

		if (type == rec_type_t::unknown)
		{
			app_logger_t::add_log("[warning] unknown rec_type_t \"" + type_str + "\", skipping chapter\r\n");
			return;
		}

		parse_chapter(arr, type, type_str);
	});

	m_loaded = true;
}

void dict_reader_t::validate_entry(record_entry_t & entry, rec_type_t type)
{
	if (type == rec_type_t::cell && entry.new_text.size() > 63)
	{
		app_logger_t::add_log(
		    "[warning] " + domain_types::type_to_str(type) + ": invalid, more than 63 bytes in " + entry.key_text +
		    "\r\n");
		return;
	}

	if (type == rec_type_t::rnam && entry.new_text.size() > 32)
	{
		app_logger_t::add_log(
		    "[warning] " + domain_types::type_to_str(type) + ": invalid, more than 32 bytes in " + entry.key_text +
		    "\r\n");
		return;
	}

	if (type == rec_type_t::fnam && entry.new_text.size() > 31)
	{
		app_logger_t::add_log(
		    "[warning] " + domain_types::type_to_str(type) + ": invalid, more than 31 bytes in " + entry.key_text +
		    "\r\n");
		return;
	}

	if (type == rec_type_t::info && entry.new_text.size() > 1024)
	{
		app_logger_t::add_log(
		    "[warning] " + domain_types::type_to_str(type) + ": invalid, more than 1024 bytes in " + entry.key_text +
		    "\r\n");
		return;
	}

	if (!dict.at(type).insert(entry))
	{
		app_logger_t::add_log("[warning] " + domain_types::type_to_str(type) + ": doubled " + entry.key_text + "\r\n");
	}
}
