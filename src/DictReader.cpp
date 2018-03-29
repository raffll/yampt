#include "DictReader.hpp"

//----------------------------------------------------------
DictReader::DictReader()
{

}

//----------------------------------------------------------
DictReader::DictReader(const DictReader& that) : status(that.status),
    name(that.name),
    name_prefix(that.name_prefix),
    counter_loaded(that.counter_loaded),
    counter_invalid(that.counter_invalid),
    counter_toolong(that.counter_toolong),
    counter_doubled(that.counter_doubled),
    counter_all(that.counter_all),
    log(that.log),
    dict(that.dict)
{

}

//----------------------------------------------------------
DictReader& DictReader::operator=(const DictReader& that)
{
    status = that.status;
    name = that.name;
    name_prefix = that.name_prefix;
    counter_loaded = that.counter_loaded;
    counter_invalid = that.counter_invalid;
    counter_toolong = that.counter_toolong;
    counter_doubled = that.counter_doubled;
    counter_all = that.counter_all;
    log = that.log;
    dict = that.dict;
    return *this;
}

//----------------------------------------------------------
DictReader::~DictReader()
{

}

//----------------------------------------------------------
void DictReader::readFile(std::string path)
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
        setName(path);
        if(!content.empty() && parseDict(content) == true)
        {
            status = true;
        }
    }
    printStatus(path);
}

//----------------------------------------------------------
void DictReader::printStatus(std::string path)
{
    if(status == false)
    {
        std::cout << "--> Error while loading " << path << " (wrong path or missing separator)!\r\n";
    }
    else
    {
        std::cout << "--> Loading " << path << "..." << std::endl;
        printLog();
    }
}

//----------------------------------------------------------
void DictReader::setName(std::string path)
{
    name = path.substr(path.find_last_of("\\/") + 1);
    name_prefix = name.substr(0, name.find_last_of("."));
}

//----------------------------------------------------------
bool DictReader::parseDict(std::string &content)
{
    size_t pos_beg = 0;
    size_t pos_mid = 0;
    size_t pos_end = 0;

    makeLogHeader();

    while(true)
    {
        pos_beg = content.find(yampt::sep[1], pos_beg);
        pos_mid = content.find(yampt::sep[2], pos_mid);
        pos_end = content.find(yampt::sep[3], pos_end);
        if(pos_beg == std::string::npos &&
           pos_mid == std::string::npos &&
           pos_end == std::string::npos)
        {
            return true;
            break;
        }
        else if(pos_beg > pos_mid ||
                pos_beg > pos_end ||
                pos_mid > pos_end ||
                pos_end == std::string::npos)
        {
            return false;
            break;
        }
        else
        {
            unique_key = content.substr(pos_beg + yampt::sep[1].size(),
                    pos_mid - pos_beg - yampt::sep[1].size());

            friendly = content.substr(pos_mid + yampt::sep[2].size(),
                    pos_end - pos_mid - yampt::sep[2].size());

            counter_all++;

            validateRecord();

            pos_beg++;
            pos_mid++;
            pos_end++;
        }
    }
}

//----------------------------------------------------------
void DictReader::validateRecord()
{
    std::string id;

    if(unique_key.size() > 4)
    {
        id = unique_key.substr(0, 4);

        if(id == "CELL")
        {
            if(friendly.size() > 63)
            {
                merger_log_ptr = &yampt::merger_log[3];
                makeLog();
                counter_toolong++;
            }
            else
            {
                insertRecord(yampt::rec_type::CELL);
            }
        }
        else if(id == "GMST")
        {
            insertRecord(yampt::rec_type::GMST);
        }
        else if(id == "DESC")
        {
            insertRecord(yampt::rec_type::DESC);
        }
        else if(id == "TEXT")
        {
            insertRecord(yampt::rec_type::TEXT);
        }
        else if(id == "INDX")
        {
            insertRecord(yampt::rec_type::INDX);
        }
        else if(id == "DIAL")
        {
            insertRecord(yampt::rec_type::DIAL);
        }
        else if(id == "BNAM")
        {
            friendly = friendly.substr(friendly.find(yampt::sep[0]) + 1);
            insertRecord(yampt::rec_type::BNAM);
        }
        else if(id == "SCTX")
        {
            friendly = friendly.substr(friendly.find(yampt::sep[0]) + 1);
            insertRecord(yampt::rec_type::SCTX);
        }
        else if(id == "RNAM")
        {
            if(friendly.size() > 32)
            {
                merger_log_ptr = &yampt::merger_log[3];
                makeLog();
                counter_toolong++;
            }
            else
            {
                insertRecord(yampt::rec_type::RNAM);
            }
        }
        else if(id == "FNAM")
        {
            if(friendly.size() > 31)
            {
                merger_log_ptr = &yampt::merger_log[3];
                makeLog();
                counter_toolong++;
            }
            else
            {
                insertRecord(yampt::rec_type::FNAM);
            }
        }
        else if(id == "INFO")
        {
            if(friendly.size() > 512)
            {
                merger_log_ptr = &yampt::merger_log[4];
                makeLog();
                insertRecord(yampt::rec_type::INFO);
            }
            else
            {
                insertRecord(yampt::rec_type::INFO);
            }
        }
        else
        {
            merger_log_ptr = &yampt::merger_log[2];
            makeLog();
            counter_invalid++;
        }
    }
    else
    {
        merger_log_ptr = &yampt::merger_log[2];
        makeLog();
        counter_invalid++;
    }
}

//----------------------------------------------------------
void DictReader::insertRecord(yampt::rec_type type)
{
    if(dict[type].insert({unique_key, friendly}).second == true)
    {
        counter_loaded++;
    }
    else
    {
        merger_log_ptr = &yampt::merger_log[1];
        makeLog();
        counter_doubled++;
    }
}

//----------------------------------------------------------
void DictReader::makeLogHeader()
{
    log += "<!-- Loading " + name + "... -->\r\n";
    log += yampt::sep_line + "\r\n";
}

//----------------------------------------------------------
void DictReader::makeLog()
{
    log += "<!-- " + *merger_log_ptr;
    if(merger_log_ptr == &yampt::merger_log[3] || merger_log_ptr == &yampt::merger_log[4])
    {
        log += " (" + std::to_string(friendly.size()) + " bytes)";
    }
    log += " -->\r\n";
    log += yampt::sep[1] + unique_key + yampt::sep[2] + friendly + yampt::sep[3] + "\r\n";
    log += yampt::sep_line + "\r\n";
}

//----------------------------------------------------------
void DictReader::printLog()
{
    std::cout << "--------------------------------------------------" << std::endl
              << "    Loaded / Doubled / Too long / Invalid /    All" << std::endl
              << "--------------------------------------------------" << std::endl
              << std::setw(10) << std::to_string(counter_loaded) << " / "
              << std::setw(7) << std::to_string(counter_doubled) << " / "
              << std::setw(8) << std::to_string(counter_toolong) << " / "
              << std::setw(7) << std::to_string(counter_invalid) << " / "
              << std::setw(6) << std::to_string(counter_all) << std::endl
              << "--------------------------------------------------" << std::endl;
}
