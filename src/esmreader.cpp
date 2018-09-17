#include "esmreader.hpp"

//----------------------------------------------------------
EsmReader::EsmReader()
    : is_loaded(false),
      unique_exist(false),
      friendly_exist(false)
{

}

//----------------------------------------------------------
EsmReader::EsmReader(const std::string &path)
    : is_loaded(false),
      unique_exist(false),
      friendly_exist(false)
{
    std::cout << "--> Loading \"" + path + "\"..." << std::endl;
    std::string content = readFile(path);
    splitFileIntoRecordColl(content, path);
    setName(path);
    setTime(path);
}

//----------------------------------------------------------
std::string EsmReader::readFile(const std::string &path)
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
void EsmReader::splitFileIntoRecordColl(const std::string &content,
                                        const std::string &path)
{
    if(content.size() > 4 &&
       content.substr(0, 4) == "TES3")
    {
        try
        {
            size_t rec_beg = 0;
            size_t rec_size = 0;
            size_t rec_end = 0;
            while(rec_end != content.size())
            {
                rec_beg = rec_end;
                rec_size = tools.convertStringByteArrayToUInt(content.substr(rec_beg + 4, 4)) + 16;
                rec_end = rec_beg + rec_size;
                rec_coll.push_back(content.substr(rec_beg, rec_size));
            }
            is_loaded = true;
        }
        catch(std::exception const& e)
        {
            std::cout << "--> Error loading \"" + path + "\" (possibly broken file or record)!" << std::endl;
            std::cout << "--> Exception: " << e.what() << std::endl;
            is_loaded = false;
        }
    }
    else
    {
        std::cout << "--> Error loading \"" + path + "\" (isn't TES3 plugin)!" << std::endl;
        is_loaded = false;
    }
}

//----------------------------------------------------------
void EsmReader::setName(const std::string &path)
{
    if(is_loaded == true)
    {
        name_full = path.substr(path.find_last_of("\\/") + 1);
        name_prefix = name_full.substr(0, name_full.find_last_of("."));
        name_suffix = name_full.substr(name_full.rfind("."));
    }
}

//----------------------------------------------------------
void EsmReader::setTime(const std::string &path)
{
    if(is_loaded == true)
    {
        time = boost::filesystem::last_write_time(path);
    }
}

//----------------------------------------------------------
void EsmReader::setRecordTo(size_t i)
{
    if(is_loaded == true)
    {
        rec = &rec_coll[i];
        rec_size = rec->size();
        rec_id = rec->substr(0, 4);
    }
}

//----------------------------------------------------------
void EsmReader::replaceRecordContent(const std::string &new_rec)
{
    if(is_loaded == true)
    {
        *rec = new_rec;
    }
}

//----------------------------------------------------------
void EsmReader::setUniqueTo(const std::string id,
                            const bool erase_null)
{
    if(is_loaded == true)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        unique_id = id;

        try
        {
            uniqueMainLoop(cur_pos, cur_size, cur_id, cur_text, erase_null);
            uniqueIfEndOfRecordReached(cur_pos);
        }
        catch(std::exception const& e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::uniqueMainLoop(std::size_t &cur_pos,
                               std::size_t &cur_size,
                               std::string &cur_id,
                               std::string &cur_text,
                               bool erase_null)
{
    while(cur_pos != rec->size())
    {
        cur_id = rec->substr(cur_pos, 4);
        cur_size = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        if(cur_id == unique_id)
        {
            if(unique_id == "DATA" && rec_id == "DIAL")
            {
                caseForDialogType(cur_pos, cur_text);
            }
            else if(unique_id == "INDX")
            {
                caseForINDX(cur_pos, cur_text);
            }
            else
            {
                caseForDefault(cur_pos, cur_size, cur_text, erase_null);
            }
            break;
        }
        cur_pos += 8 + cur_size;
    }
}

//----------------------------------------------------------
void EsmReader::caseForDialogType(std::size_t &cur_pos,
                                  std::string &cur_text)
{
    size_t type = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 8, 1));
    cur_text = yampt::dialog_type[type];
    unique_text = cur_text;
    unique_exist = true;
}

//----------------------------------------------------------
void EsmReader::caseForINDX(std::size_t &cur_pos,
                            std::string &cur_text)
{
    size_t indx = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 8, 4));
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(3) << indx;
    cur_text = ss.str();
    unique_text = cur_text;
    unique_exist = true;
}

//----------------------------------------------------------
void EsmReader::caseForDefault(std::size_t &cur_pos,
                               std::size_t &cur_size,
                               std::string &cur_text,
                               bool erase_null)
{
    cur_text = rec->substr(cur_pos + 8, cur_size);
    if(erase_null == true)
    {
        cur_text = tools.eraseNullChars(cur_text);
    }
    unique_text = cur_text;
    unique_exist = true;
}

//----------------------------------------------------------
void EsmReader::uniqueIfEndOfRecordReached(std::size_t &cur_pos)
{
    if(cur_pos == rec->size())
    {
        unique_text = "Unique id " + unique_id + " not found!";
        unique_exist = false;
    }
}

//----------------------------------------------------------
void EsmReader::setFriendlyTo(const std::string &id,
                              const bool erase_null)
{
    if(is_loaded == true)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        friendly_id = id;
        friendly_counter = 0;

        try
        {
            friendlyMainLoop(cur_pos, cur_size, cur_id, cur_text, erase_null);
            friendlyIfEndOfRecordReached(cur_pos);
        }
        catch(std::exception const& e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::setNextFriendlyTo(const std::string &id,
                                  const bool erase_null)
{
    if(is_loaded == true &&
       friendly_exist == true)
    {
        size_t cur_pos;
        size_t cur_size;
        std::string cur_id;
        std::string cur_text;
        friendly_id = id;

        cur_pos = friendly_pos;
        cur_size = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        cur_pos += 8 + cur_size;
        friendly_counter++;

        try
        {
            friendlyMainLoop(cur_pos, cur_size, cur_id, cur_text, erase_null);
            friendlyIfEndOfRecordReached(cur_pos);
        }
        catch(std::exception const& e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::friendlyMainLoop(std::size_t &cur_pos,
                                 std::size_t &cur_size,
                                 std::string &cur_id,
                                 std::string &cur_text,
                                 bool erase_null)
{
    while(cur_pos != rec->size())
    {
        cur_id = rec->substr(cur_pos, 4);
        cur_size = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        if(cur_id == friendly_id)
        {
            cur_text = rec->substr(cur_pos + 8, cur_size);
            if(erase_null == true)
            {
                cur_text = tools.eraseNullChars(cur_text);
            }
            friendly_text = cur_text;
            friendly_pos = cur_pos;
            friendly_size = cur_size;
            friendly_exist = true;
            break;
        }
        cur_pos += 8 + cur_size;
    }
}

//----------------------------------------------------------
void EsmReader::friendlyIfEndOfRecordReached(std::size_t &cur_pos)
{
    if(cur_pos == rec->size())
    {
        friendly_text = "Friendly id " + friendly_id + " not found!";
        friendly_pos = cur_pos;
        friendly_size = 0;
        friendly_exist = false;
    }
}

//----------------------------------------------------------
void EsmReader::handleException(std::exception const& e)
{
    std::string cur_rec = tools.replaceNonReadableCharsWithDot(*rec);
    std::cout << "--> Error in function (possibly broken record)!" << std::endl;
    std::cout << cur_rec << std::endl;
    std::cout << "--> Exception: " << e.what() << std::endl;
    is_loaded = false;
}
