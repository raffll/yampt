#include "dictreader.hpp"

//----------------------------------------------------------
DictReader::DictReader(const std::string & path)
{
    dict = Tools::initializeDict();

    std::string content = Tools::readFile(path);
    if (!content.empty())
    {
        parseDict(content, path);
        setName(path);
        printSummaryLog();
    }
}

//----------------------------------------------------------
DictReader::DictReader(const DictReader & that)
    : name_full(that.name_full)
    , name_prefix(that.name_prefix)
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
    name_full = that.name_full;
    name_prefix = that.name_prefix;
    dict = that.dict;
    is_loaded = that.is_loaded;
    counter_loaded = that.counter_loaded;
    counter_invalid = that.counter_invalid;
    counter_doubled = that.counter_doubled;
    counter_all = that.counter_all;
    return *this;
}

//----------------------------------------------------------
void DictReader::setName(const std::string & path)
{
    name_full = path.substr(path.find_last_of("\\/") + 1);
    name_prefix = name_full.substr(0, name_full.find_last_of("."));
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
            val_text = content.substr(pos_beg, pos_end - pos_beg);

            type_name = found[1].str();
            key_text = found[2].str();

            validateRecord();
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
void DictReader::validateRecord()
{
    if (type_name == "CELL")
    {
        insertCELL();
    }
    else if (type_name == "GMST")
    {
        insertRecord(Tools::RecType::GMST);
    }
    else if (type_name == "DESC")
    {
        insertRecord(Tools::RecType::DESC);
    }
    else if (type_name == "TEXT")
    {
        insertRecord(Tools::RecType::TEXT);
    }
    else if (type_name == "INDX")
    {
        insertRecord(Tools::RecType::INDX);
    }
    else if (type_name == "DIAL")
    {
        insertRecord(Tools::RecType::DIAL);
    }
    else if (type_name == "BNAM")
    {
        insertRecord(Tools::RecType::BNAM);
    }
    else if (type_name == "SCTX")
    {
        insertRecord(Tools::RecType::SCTX);
    }
    else if (type_name == "RNAM")
    {
        insertRNAM();
    }
    else if (type_name == "FNAM")
    {
        insertFNAM();
    }
    else if (type_name == "INFO")
    {
        insertINFO();
    }
    else if (type_name == "Glossary")
    {
        insertRecord(Tools::RecType::Glossary);
    }
    else
    {
        Tools::addLog(type_name + ": invalid id in" + key_text + "\r\n");
        counter_invalid++;
    }
}

//----------------------------------------------------------
void DictReader::insertRecord(
    const Tools::RecType type)
{
    if (dict[type].insert({ key_text, val_text }).second)
    {
        counter_loaded++;
    }
    else
    {
        Tools::addLog(Tools::getTypeName(type) + ": doubled " + key_text + "\r\n");
        counter_doubled++;
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

//----------------------------------------------------------
void DictReader::insertCELL()
{
    if (val_text.size() > 63)
    {
        Tools::addLog(type_name + ": invalid, more than 63 bytes in " + key_text + "\r\n");
        counter_invalid++;
    }
    else
    {
        insertRecord(Tools::RecType::CELL);
    }
}

//----------------------------------------------------------
void DictReader::insertRNAM()
{
    if (val_text.size() > 32)
    {
        Tools::addLog(type_name + ": invalid, more than 32 bytes in " + key_text + "\r\n");
        counter_invalid++;
    }
    else
    {
        insertRecord(Tools::RecType::RNAM);
    }
}

//----------------------------------------------------------
void DictReader::insertFNAM()
{
    if (val_text.size() > 31)
    {
        Tools::addLog(type_name + ": invalid, more than 31 bytes in " + key_text + "\r\n");
        counter_invalid++;
    }
    else
    {
        insertRecord(Tools::RecType::FNAM);
    }
}

//----------------------------------------------------------
void DictReader::insertINFO()
{
    if (val_text.size() > 1024)
    {
        Tools::addLog(type_name + ": invalid, more than 1024 bytes in " + key_text + "\r\n");
        counter_invalid++;
    }
    else if (val_text.size() > 512)
    {
        Tools::addLog(type_name + ": valid, but more than 512 bytes in " + key_text + "\r\n", true);
        insertRecord(Tools::RecType::INFO);
    }
    else
    {
        insertRecord(Tools::RecType::INFO);
    }
}
