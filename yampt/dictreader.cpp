#include "dictreader.hpp"

DictReader::DictReader(const std::string & path)
{
    dict = tools_t::initializeDict();

    std::string content = tools_t::readFile(path);
    if (!content.empty())
    {
        parseJson(content, path);
        name.setName(path);
    }
}

void DictReader::parseJson(const std::string & content, const std::string & path)
{
    nlohmann::json root;
    try
    {
        root = nlohmann::json::parse(content);
    }
    catch (const nlohmann::json::parse_error & e)
    {
        tools_t::addLog("--> Error parsing \"" + path + "\"!\r\n");
        tools_t::addLog("--> " + std::string(e.what()) + "\r\n");
        is_loaded = false;
        return;
    }

    for (auto it = root.begin(); it != root.end(); ++it)
    {
        const std::string & type_str = it.key();
        tools_t::rec_type_t type = tools_t::str2Type(type_str);

        if (type == tools_t::rec_type_t::Unknown)
        {
            tools_t::addLog("Warning: unknown rec_type_t \"" + type_str + "\", skipping chapter\r\n");
            continue;
        }

        const auto & records_array = it.value();
        for (const auto & record_json : records_array)
        {
            if (!record_json.contains("id"))
            {
                tools_t::addLog("Warning: record missing \"id\" field in chapter " + type_str + ", skipping\r\n");
                continue;
            }

            tools_t::RecordEntry entry;
            entry.key_text = record_json.value("id", "");
            entry.old_text = record_json.value("old", "");
            entry.new_text = record_json.value("new", "");
            entry.status = record_json.value("status", "");

            validateEntry(entry, type);
        }
    }

    is_loaded = true;
}

void DictReader::validateEntry(tools_t::RecordEntry & entry, tools_t::rec_type_t type)
{
    if (type == tools_t::rec_type_t::CELL && entry.new_text.size() > 63)
    {
        tools_t::addLog(tools_t::type2Str(type) + ": invalid, more than 63 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (type == tools_t::rec_type_t::RNAM && entry.new_text.size() > 32)
    {
        tools_t::addLog(tools_t::type2Str(type) + ": invalid, more than 32 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (type == tools_t::rec_type_t::FNAM && entry.new_text.size() > 31)
    {
        tools_t::addLog(tools_t::type2Str(type) + ": invalid, more than 31 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (type == tools_t::rec_type_t::INFO && entry.new_text.size() > 1024)
    {
        tools_t::addLog(tools_t::type2Str(type) + ": invalid, more than 1024 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (!dict.at(type).insert(entry))
    {
        tools_t::addLog(tools_t::type2Str(type) + ": doubled " + entry.key_text + "\r\n");
    }
}
