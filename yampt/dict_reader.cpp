#include "dict_reader.hpp"
#include "json_reader.hpp"

dict_reader_t::dict_reader_t(const std::string & path)
{
	dict = tools_t::initialize_dict();

	std::string content = tools_t::read_file(path);
	if (!content.empty())
	{
		parse_json(content, path);
		name.set_name(path);
	}
}

void dict_reader_t::parse_json(const std::string & content, const std::string & path)
{
	json_reader_t reader;
	if (!reader.parse(content.data(), content.size(), true))
	{
		tools_t::add_log("[error] parsing \"" + path + "\"\r\n");
		tools_t::add_log("[error] " + reader.get_error() + "\r\n");
		loaded_ = false;
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

		if (type_str == "npc_flag" || type_str == "NPC_FLAG")
		{
			tools_t::add_log("[warning] legacy npc_flag chapter in \"" + path + "\", skipping\r\n");
			return;
		}

		tools_t::rec_type_t type = tools_t::str_to_type(type_str);

		if (type == tools_t::rec_type_t::unknown)
		{
			tools_t::add_log("[warning] unknown rec_type_t \"" + type_str + "\", skipping chapter\r\n");
			return;
		}

		json_reader_t::foreach_arr(
		    arr,
		    [&](size_t, yyjson_val * record)
		{
			std::string key_text = json_reader_t::get_string(record, "key", "");
			if (key_text.empty())
			{
				tools_t::add_log("[warning] record missing \"key\" field in chapter " + type_str + ", skipping\r\n");
				return;
			}

			tools_t::record_entry_t entry;
			entry.key_text = std::move(key_text);
			entry.old_text = json_reader_t::get_string(record, "old", "");
			entry.new_text = json_reader_t::get_string(record, "new", "");
			entry.status = json_reader_t::get_string(record, "status", "");

			if (type == tools_t::rec_type_t::info)
			{
				entry.speaker = json_reader_t::get_string(record, "speaker", "");
				entry.speaker_name = json_reader_t::get_string(record, "speaker_name", "");
				entry.gender = json_reader_t::get_string(record, "gender", "");
			}

			validate_entry(entry, type);
		});
	});

	loaded_ = true;
}

void dict_reader_t::validate_entry(tools_t::record_entry_t & entry, tools_t::rec_type_t type)
{
	if (type == tools_t::rec_type_t::cell && entry.new_text.size() > 63)
	{
		tools_t::add_log(
		    "[warning] " + tools_t::type_to_str(type) + ": invalid, more than 63 bytes in " + entry.key_text + "\r\n");
		return;
	}

	if (type == tools_t::rec_type_t::rnam && entry.new_text.size() > 32)
	{
		tools_t::add_log(
		    "[warning] " + tools_t::type_to_str(type) + ": invalid, more than 32 bytes in " + entry.key_text + "\r\n");
		return;
	}

	if (type == tools_t::rec_type_t::fnam && entry.new_text.size() > 31)
	{
		tools_t::add_log(
		    "[warning] " + tools_t::type_to_str(type) + ": invalid, more than 31 bytes in " + entry.key_text + "\r\n");
		return;
	}

	if (type == tools_t::rec_type_t::info && entry.new_text.size() > 1024)
	{
		tools_t::add_log(
		    "[warning] " + tools_t::type_to_str(type) + ": invalid, more than 1024 bytes in " + entry.key_text +
		    "\r\n");
		return;
	}

	if (!dict.at(type).insert(entry))
	{
		tools_t::add_log("[warning] " + tools_t::type_to_str(type) + ": doubled " + entry.key_text + "\r\n");
	}
}
