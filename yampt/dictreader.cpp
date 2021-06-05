#include "dictreader.hpp"

//----------------------------------------------------------
DictReader::DictReader(const std::string & path)
{
    dict = Tools::initializeDict();

    std::string content = Tools::readFile(path);
    if (!content.empty())
    {
        parseDict(content, path);
        name.setName(path);
        printSummaryLog();
    }
}

//----------------------------------------------------------
DictReader::DictReader(const DictReader & that)
    : name(that.name)
    , dict(that.dict)
    , is_loaded(that.is_loaded)
    , counter_loaded(that.counter_loaded)
    , counter_invalid(that.counter_invalid)
    , counter_doubled(that.counter_doubled)
    , counter_all(that.counter_all)
{

}

//----------------------------------------------------------
DictReader & DictReader::operator=(const DictReader & that)
{
    name = that.name;
    dict = that.dict;
    is_loaded = that.is_loaded;
    counter_loaded = that.counter_loaded;
    counter_invalid = that.counter_invalid;
    counter_doubled = that.counter_doubled;
    counter_all = that.counter_all;
    return *this;
}

//----------------------------------------------------------
void DictReader::parseDict(
    const std::string & content,
    const std::string & path)
{
    try
    {
        size_t pos_beg;
        size_t pos_end;
        std::string re_str = Tools::sep[1] + "(.*?)" + Tools::sep[2] + "\\s*" + Tools::sep[3] + "(.*?)" + Tools::sep[4];
        std::regex re(re_str);
        std::smatch found;
        std::sregex_iterator next(content.begin(), content.end(), re);
        std::sregex_iterator end;
        while (next != end)
        {
            found = *next;

            // no multiline in regex :(
            pos_beg = content.find(Tools::sep[5], found.position(2)) + Tools::sep[5].size();
            pos_end = content.find(Tools::sep[6], pos_beg);
            const auto & val_text = content.substr(pos_beg, pos_end - pos_beg);

            const auto & type = Tools::str2Type(found[1].str());
            const auto & key_text = found[2].str();

            validateEntry({ key_text, val_text, type });
            counter_all++;
            next++;
        }
        is_loaded = true;
    }
    catch (const std::exception & e)
    {
        Tools::addLog("--> Error parsing \"" + path + "\" (possibly broken dictionary)!\r\n");
        Tools::addLog("--> Exception: " + std::string(e.what()) + "\r\n");
        is_loaded = false;
    }
}

//----------------------------------------------------------
void DictReader::validateEntry(const Tools::Entry & entry)
{
    if (entry.type == Tools::RecType::CELL)
    {
        insertCELL(entry);
    }
    else if (entry.type == Tools::RecType::RNAM)
    {
        insertRNAM(entry);
    }
    else if (entry.type == Tools::RecType::FNAM)
    {
        insertFNAM(entry);
    }
    else if (entry.type == Tools::RecType::INFO)
    {
        insertINFO(entry);
    }
    else if (entry.type == Tools::RecType::Unknown)
    {
        Tools::addLog("Warning: invalid id in " + entry.key_text + "\r\n");
        counter_invalid++;
    }
    else
    {
        insertRecord(entry);
    }
}

//----------------------------------------------------------
void DictReader::insertRecord(const Tools::Entry & entry)
{
    if (dict.at(entry.type).insert({ entry.key_text, entry.val_text }).second)
    {
        counter_loaded++;
    }
    else
    {
        Tools::addLog(Tools::type2Str(entry.type) + ": doubled " + entry.key_text + "\r\n");
        counter_doubled++;
    }
}

//----------------------------------------------------------
void DictReader::insertCELL(const Tools::Entry & entry)
{
    if (entry.val_text.size() > 63)
    {
        Tools::addLog(Tools::type2Str(entry.type) + ": invalid, more than 63 bytes in " + entry.key_text + "\r\n");
        counter_invalid++;
    }
    else
    {
        insertRecord(entry);
    }
}

//----------------------------------------------------------
void DictReader::insertRNAM(const Tools::Entry & entry)
{
    if (entry.val_text.size() > 32)
    {
        Tools::addLog(Tools::type2Str(entry.type) + ": invalid, more than 32 bytes in " + entry.key_text + "\r\n");
        counter_invalid++;
    }
    else
    {
        insertRecord(entry);
    }
}

//----------------------------------------------------------
void DictReader::insertFNAM(const Tools::Entry & entry)
{
    if (entry.val_text.size() > 31)
    {
        Tools::addLog(Tools::type2Str(entry.type) + ": invalid, more than 31 bytes in " + entry.key_text + "\r\n");
        counter_invalid++;
    }
    else
    {
        insertRecord(entry);
    }
}

//----------------------------------------------------------
void DictReader::insertINFO(const Tools::Entry & entry)
{
    if (entry.val_text.size() > 1024)
    {
        Tools::addLog(Tools::type2Str(entry.type) + ": invalid, more than 1024 bytes in " + entry.key_text + "\r\n");
        counter_invalid++;
    }
    else if (entry.val_text.size() > 512)
    {
        Tools::addLog(Tools::type2Str(entry.type) + ": valid, but more than 512 bytes in " + entry.key_text + "\r\n", true);
        insertRecord(entry);
    }
    else
    {
        insertRecord(entry);
    }
}

//----------------------------------------------------------
void DictReader::printSummaryLog()
{
    std::ostringstream ss;
    ss
        << "---------------------------------------" << std::endl
        << "    Loaded / Doubled / Invalid /    All" << std::endl
        << "---------------------------------------" << std::endl
        << std::setw(10) << std::to_string(counter_loaded) << " / "
        << std::setw(7) << std::to_string(counter_doubled) << " / "
        << std::setw(7) << std::to_string(counter_invalid) << " / "
        << std::setw(6) << std::to_string(counter_all) << std::endl
        << "---------------------------------------" << std::endl;

    Tools::addLog(ss.str());
}
