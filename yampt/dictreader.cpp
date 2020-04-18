#include "dictreader.hpp"

//----------------------------------------------------------
DictReader::DictReader(const std::string &path) :
    is_loaded(false)
{
    std::string content = tools.readFile(path);
    parseDict(content, path);
    setName(path);
    printSummaryLog();
}

//----------------------------------------------------------
DictReader::DictReader(const DictReader& that)
    : name_full(that.name_full),
      name_prefix(that.name_prefix),
      dict(that.dict),
      is_loaded(that.is_loaded),
      counter_loaded(that.counter_loaded),
      counter_invalid(that.counter_invalid),
      counter_doubled(that.counter_doubled),
      counter_all(that.counter_all)
{

}

//----------------------------------------------------------
DictReader& DictReader::operator=(const DictReader& that)
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
DictReader::~DictReader()
{

}

//----------------------------------------------------------
void DictReader::setName(const std::string &path)
{
    name_full = path.substr(path.find_last_of("\\/") + 1);
    name_prefix = name_full.substr(0, name_full.find_last_of("."));
}

//----------------------------------------------------------
void DictReader::parseDict(const std::string &content,
                           const std::string &path)
{
    try
    {
        std::string friendly_text;
        size_t pos_beg;
        size_t pos_end;
        std::string re_str = Tools::sep[1] + "(.*?)" + Tools::sep[2] + "\\s*" + Tools::sep[3] + "(.*?)" + Tools::sep[4];
        std::regex re(re_str);
        std::smatch found;
        std::sregex_iterator next(content.begin(), content.end(), re);
        std::sregex_iterator end;
        while(next != end)
        {
            found = *next;

            // no multiline in regex :(
            pos_beg = content.find(Tools::sep[5], found.position(2)) + Tools::sep[5].size();
            pos_end = content.find(Tools::sep[6], pos_beg);
            friendly_text = content.substr(pos_beg, pos_end - pos_beg);

            validateRecord(found[1].str(), found[2].str(), friendly_text);
            counter_all++;
            next++;
        }
        is_loaded = true;
    }
    catch(std::exception const& e)
    {
        std::cout << "--> Error parsing \"" + path + "\" (possibly broken dictionary)!" << std::endl;
        std::cout << "--> Exception: " << e.what() << std::endl;
        is_loaded = false;
    }
}

//----------------------------------------------------------
void DictReader::validateRecord(const std::string &id,
                                const std::string &unique_text,
                                const std::string &friendly_text)
{
    if(id == "CELL")
    {
        if(friendly_text.size() > 63)
        {
            tools.addLog("Too long, more than 63 bytes in " + id + ": " + unique_text);
            counter_invalid++;
        }
        else
        {
            insertRecord(Tools::RecType::CELL, unique_text, friendly_text);
        }
    }
    else if(id == "GMST")
    {
        insertRecord(Tools::RecType::GMST, unique_text, friendly_text);
    }
    else if(id == "DESC")
    {
        insertRecord(Tools::RecType::DESC, unique_text, friendly_text);
    }
    else if(id == "TEXT")
    {
        insertRecord(Tools::RecType::TEXT, unique_text, friendly_text);
    }
    else if(id == "INDX")
    {
        insertRecord(Tools::RecType::INDX, unique_text, friendly_text);
    }
    else if(id == "DIAL")
    {
        insertRecord(Tools::RecType::DIAL, unique_text, friendly_text);
    }
    else if(id == "BNAM")
    {
        insertRecord(Tools::RecType::BNAM, unique_text, friendly_text);
    }
    else if(id == "SCTX")
    {
        insertRecord(Tools::RecType::SCTX, unique_text, friendly_text);
    }
    else if(id == "RNAM")
    {
        if(friendly_text.size() > 32)
        {
            tools.addLog("Too long, more than 32 bytes in " + id + ": " + unique_text);
            counter_invalid++;
        }
        else
        {
            insertRecord(Tools::RecType::RNAM, unique_text, friendly_text);
        }
    }
    else if(id == "FNAM")
    {
        if(friendly_text.size() > 31)
        {
            tools.addLog("Too long, more than 31 bytes in " + id + ": " + unique_text);
            counter_invalid++;
        }
        else
        {
            insertRecord(Tools::RecType::FNAM, unique_text, friendly_text);
        }
    }
    else if(id == "INFO")
    {
        if(friendly_text.size() > 512)
        {
            tools.addLog("Ok, but more than 512 bytes in " + id + ": " + unique_text);
            insertRecord(Tools::RecType::INFO, unique_text, friendly_text);
        }
        else
        {
            insertRecord(Tools::RecType::INFO, unique_text, friendly_text);
        }
    }
    else
    {
        tools.addLog("Invalid id " + id + ": " + unique_text);
        counter_invalid++;
    }

}

//----------------------------------------------------------
void DictReader::insertRecord(const Tools::RecType type,
                              const std::string &unique_text,
                              const std::string &friendly_text)
{
    if(dict[type].insert({unique_text, friendly_text}).second == true)
    {
        counter_loaded++;
    }
    else
    {
        tools.addLog("Doubled " + Tools::type_name[type] + ": " + unique_text);
        counter_doubled++;
    }
}

//----------------------------------------------------------
void DictReader::printSummaryLog()
{
    std::cout << "---------------------------------------" << std::endl
              << "    Loaded / Doubled / Invalid /    All" << std::endl
              << "---------------------------------------" << std::endl
              << std::setw(10) << std::to_string(counter_loaded) << " / "
              << std::setw(7) << std::to_string(counter_doubled) << " / "
              << std::setw(7) << std::to_string(counter_invalid) << " / "
              << std::setw(6) << std::to_string(counter_all) << std::endl
              << "---------------------------------------" << std::endl;
    if(!tools.getLog().empty())
    {
        std::cout << tools.getLog()
                  << "---------------------------------------" << std::endl;
        tools.clearLog();
    }
}
