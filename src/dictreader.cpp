#include "dictreader.hpp"

//----------------------------------------------------------
DictReader::DictReader()
{

}

//----------------------------------------------------------
DictReader::DictReader(const DictReader& that)
    : name_full(that.name_full),
      name_prefix(that.name_prefix),
      dict(that.dict),
      log(that.log),
      status(that.status),
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
    log = that.log;
    status = that.status;
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
void DictReader::readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if(file)
    {
        std::string content;
        char buffer[16384];
        size_t size = file.tellg();
        content.reserve(size);
        std::streamsize chars_read;
        while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
        {
            content.append(buffer, chars_read);
        }
        parseDict(content, path);
    }
    else
    {
        std::cout << "--> Error while loading " + path + " (wrong path)!" << std::endl;
        status = false;
    }
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

        std::regex re("<id>(.*?)</id>\\s*<key>(.*?)</key>");
        std::smatch found;
        std::sregex_iterator next(content.begin(), content.end(), re);
        std::sregex_iterator end;
        while(next != end)
        {
            found = *next;

            // no multiline in regex :(
            pos_beg = content.find("<val>", found.position(2)) + 5;
            pos_end = content.find("</val>", pos_beg);
            friendly_text = content.substr(pos_beg, pos_end - pos_beg);

            validateRecord(found[1].str(), found[2].str(), friendly_text);
            counter_all++;
            next++;
        }
        std::cout << "--> Loading " + path + "..." << std::endl;
        printLog();
        setName(path);
        status = true;
    }
    catch(std::exception const& e)
    {
        std::cout << "--> Error in function parseDict() (possibly broken dictionary)!" << std::endl;
        std::cout << "--> Exception: " << e.what() << std::endl;
        status = false;
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
            makeLog(id, unique_text, friendly_text, "Too long, more than 63 bytes!");
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
            makeLog(id, unique_text, friendly_text, "Too long, more than 32 bytes!");
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
            makeLog(id, unique_text, friendly_text, "Too long, more than 31 bytes!");
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
            makeLog(id, unique_text, friendly_text, "Ok, but more than 512 bytes!");
            insertRecord(yampt::rec_type::INFO, unique_text, friendly_text);
        }
        else
        {
            insertRecord(yampt::rec_type::INFO, unique_text, friendly_text);
        }
    }
    else
    {
        makeLog(id, unique_text, friendly_text, "Invalid record!");
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
        makeLog(yampt::type_name[type], unique_text, friendly_text, "Doubled record!");
        counter_doubled++;
    }
}

//----------------------------------------------------------
void DictReader::makeLog(const std::string &id,
                         const std::string &unique_text,
                         const std::string &friendly_text,
                         const std::string &comment)
{
    log += "<log>\r\n";
    log += "\t<file>" + name_full + "</file>\r\n";
    log += "\t<status>" + comment + "</status>\r\n";
    log += "\t<id>" + id + "</id>\r\n";
    log += "\t<key>" + unique_text + "</key>\r\n";
    log += "\t<val>" + friendly_text + "</val>\r\n";
    log += "<log>\r\n";
}

//----------------------------------------------------------
void DictReader::printLog()
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
