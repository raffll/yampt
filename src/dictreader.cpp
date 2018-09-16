#include "dictreader.hpp"

//----------------------------------------------------------
DictReader::DictReader(const std::string &path) :
    is_loaded(false)
{
    std::cout << "--> Loading \"" + path + "\"..." << std::endl;
    std::string content = readFile(path);
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
std::string DictReader::readFile(const std::string &path)
{
    std::string content;
    std::ifstream file(path, std::ios::binary);
    if(file)
    {
        char buffer[16384];
        size_t size = file.tellg();
        content.reserve(size);
        std::streamsize chars_read;
        while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
        {
            content.append(buffer, chars_read);
        }
    }
    else
    {
        std::cout << "--> Error loading \"" + path + "\" (wrong path)!" << std::endl;
    }
    return content;
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
        std::string re_str = yampt::sep[1] + "(.*?)" + yampt::sep[2] + "\\s*" + yampt::sep[3] + "(.*?)" + yampt::sep[4];
        std::regex re(re_str);
        std::smatch found;
        std::sregex_iterator next(content.begin(), content.end(), re);
        std::sregex_iterator end;
        while(next != end)
        {
            found = *next;

            // no multiline in regex :(
            pos_beg = content.find(yampt::sep[5], found.position(2)) + yampt::sep[5].size();
            pos_end = content.find(yampt::sep[6], pos_beg);
            friendly_text = content.substr(pos_beg, pos_end - pos_beg);

            validateRecord(found[1].str(), found[2].str(), friendly_text);
            counter_all++;
            next++;
        }
        is_loaded = true;
    }
    catch(std::exception const& e)
    {
        std::cout << "--> Error loading \"" + path + "\" (possibly broken dictionary)!" << std::endl;
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
            printWarningLog(id, unique_text, "Too long, more than 63 bytes!");
            counter_invalid++;
        }
        else
        {
            insertRecord(yampt::rec_type::CELL, unique_text, friendly_text);
        }
    }
    else if(id == "GMST")
    {
        insertRecord(yampt::rec_type::GMST, unique_text, friendly_text);
    }
    else if(id == "DESC")
    {
        insertRecord(yampt::rec_type::DESC, unique_text, friendly_text);
    }
    else if(id == "TEXT")
    {
        insertRecord(yampt::rec_type::TEXT, unique_text, friendly_text);
    }
    else if(id == "INDX")
    {
        insertRecord(yampt::rec_type::INDX, unique_text, friendly_text);
    }
    else if(id == "DIAL")
    {
        insertRecord(yampt::rec_type::DIAL, unique_text, friendly_text);
    }
    else if(id == "BNAM")
    {
        insertRecord(yampt::rec_type::BNAM, unique_text, friendly_text);
    }
    else if(id == "SCTX")
    {
        insertRecord(yampt::rec_type::SCTX, unique_text, friendly_text);
    }
    else if(id == "RNAM")
    {
        if(friendly_text.size() > 32)
        {
            printWarningLog(id, unique_text, "Too long, more than 32 bytes!");
            counter_invalid++;
        }
        else
        {
            insertRecord(yampt::rec_type::RNAM, unique_text, friendly_text);
        }
    }
    else if(id == "FNAM")
    {
        if(friendly_text.size() > 31)
        {
            printWarningLog(id, unique_text, "Too long, more than 31 bytes!");
            counter_invalid++;
        }
        else
        {
            insertRecord(yampt::rec_type::FNAM, unique_text, friendly_text);
        }
    }
    else if(id == "INFO")
    {
        if(friendly_text.size() > 512)
        {
            printWarningLog(id, unique_text, "Ok, but more than 512 bytes!");
            insertRecord(yampt::rec_type::INFO, unique_text, friendly_text);
        }
        else
        {
            insertRecord(yampt::rec_type::INFO, unique_text, friendly_text);
        }
    }
    else
    {
        printWarningLog(id, unique_text, "Invalid record!");
        counter_invalid++;
    }

}

//----------------------------------------------------------
void DictReader::insertRecord(const yampt::rec_type type,
                              const std::string &unique_text,
                              const std::string &friendly_text)
{
    if(dict[type].insert({unique_text, friendly_text}).second == true)
    {
        counter_loaded++;
    }
    else
    {
        printWarningLog(yampt::type_name[type], unique_text, "Doubled record!");
        counter_doubled++;
    }
}

//----------------------------------------------------------
void DictReader::printWarningLog(const std::string &id,
                                 const std::string &unique_text,
                                 const std::string &comment)
{
    std::cerr << "Warning! " << id << ": " << unique_text << " --> " << comment << std::endl;
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
}
