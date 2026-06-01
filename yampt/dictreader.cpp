#include "dictreader.hpp"

DictReader::DictReader(const std::string & path)
{
    dict = Tools::initializeDict();

    std::string content = Tools::readFile(path);
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
        Tools::addLog("--> Error parsing \"" + path + "\"!\r\n");
        Tools::addLog("--> " + std::string(e.what()) + "\r\n");
        is_loaded = false;
        return;
    }

    for (auto it = root.begin(); it != root.end(); ++it)
    {
        const std::string & type_str = it.key();
        Tools::RecType type = Tools::str2Type(type_str);

        if (type == Tools::RecType::Unknown)
        {
            Tools::addLog("Warning: unknown RecType \"" + type_str + "\", skipping chapter\r\n");
            continue;
        }

        const auto & records_array = it.value();
        for (const auto & record_json : records_array)
        {
            if (!record_json.contains("id"))
            {
                Tools::addLog("Warning: record missing \"id\" field in chapter " + type_str + ", skipping\r\n");
                continue;
            }

            Tools::RecordEntry entry;
            entry.key_text = record_json.value("id", "");
            entry.old_text = record_json.value("old", "");
            entry.new_text = record_json.value("new", "");
            entry.status = record_json.value("status", "");

            validateEntry(entry, type);
        }
    }

    is_loaded = true;
}

void DictReader::validateEntry(Tools::RecordEntry & entry, Tools::RecType type)
{
    if (type == Tools::RecType::CELL && entry.new_text.size() > 63)
    {
        Tools::addLog(Tools::type2Str(type) + ": invalid, more than 63 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (type == Tools::RecType::RNAM && entry.new_text.size() > 32)
    {
        Tools::addLog(Tools::type2Str(type) + ": invalid, more than 32 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (type == Tools::RecType::FNAM && entry.new_text.size() > 31)
    {
        Tools::addLog(Tools::type2Str(type) + ": invalid, more than 31 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (type == Tools::RecType::INFO && entry.new_text.size() > 1024)
    {
        Tools::addLog(Tools::type2Str(type) + ": invalid, more than 1024 bytes in " + entry.key_text + "\r\n");
        return;
    }

    if (!dict.at(type).insert(entry))
    {
        Tools::addLog(Tools::type2Str(type) + ": doubled " + entry.key_text + "\r\n");
    }
}
