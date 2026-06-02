#include "dict_reader.hpp"

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
	nlohmann::json root;
	try
	{
		root = nlohmann::json::parse(content);
	}
	catch (const nlohmann::json::parse_error & e)
	{
		tools_t::add_log("[error] parsing \"" + path + "\"\r\n");
		tools_t::add_log("[error] " + std::string(e.what()) + "\r\n");
		loaded_ = false;
		return;
	}

	for (auto it = root.begin(); it != root.end(); ++it)
	{
		const std::string & type_str = it.key();
		tools_t::rec_type_t type = tools_t::str_to_type(type_str);

		if (type == tools_t::rec_type_t::unknown)
		{
			tools_t::add_log("[warn] unknown rec_type_t \"" + type_str + "\", skipping chapter\r\n");
			continue;
		}

		const auto & records_array = it.value();
		for (const auto & record_json : records_array)
		{
			if (!record_json.contains("key"))
			{
				tools_t::add_log("[warn] record missing \"key\" field in chapter " + type_str + ", skipping\r\n");
				continue;
			}

			tools_t::record_entry_t entry;
			entry.key_text = record_json.value("key", "");
			entry.old_text = record_json.value("old", "");
			entry.new_text = record_json.value("new", "");
			entry.status = record_json.value("status", "");

			validate_entry(entry, type);
		}
	}

	loaded_ = true;
}

void dict_reader_t::validate_entry(tools_t::record_entry_t & entry, tools_t::rec_type_t type)
{
	if (type == tools_t::rec_type_t::cell && entry.new_text.size() > 63)
	{
		tools_t::add_log(
		    "[warn] " + tools_t::type_to_str(type) + ": invalid, more than 63 bytes in " + entry.key_text + "\r\n");
		return;
	}

	if (type == tools_t::rec_type_t::rnam && entry.new_text.size() > 32)
	{
		tools_t::add_log(
		    "[warn] " + tools_t::type_to_str(type) + ": invalid, more than 32 bytes in " + entry.key_text + "\r\n");
		return;
	}

	if (type == tools_t::rec_type_t::fnam && entry.new_text.size() > 31)
	{
		tools_t::add_log(
		    "[warn] " + tools_t::type_to_str(type) + ": invalid, more than 31 bytes in " + entry.key_text + "\r\n");
		return;
	}

	if (type == tools_t::rec_type_t::info && entry.new_text.size() > 1024)
	{
		tools_t::add_log(
		    "[warn] " + tools_t::type_to_str(type) + ": invalid, more than 1024 bytes in " + entry.key_text + "\r\n");
		return;
	}

	if (!dict.at(type).insert(entry))
	{
		tools_t::add_log("[warn] " + tools_t::type_to_str(type) + ": doubled " + entry.key_text + "\r\n");
	}
}
