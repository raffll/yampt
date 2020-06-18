#include "esmreader.hpp"

//----------------------------------------------------------
EsmReader::EsmReader(const std::string & path)
{
    std::string content = Tools::readFile(path);

    if (!content.empty())
        splitFile(content, path);

    setName(path);
    setTime(path);
}

//----------------------------------------------------------
void EsmReader::splitFile(
    const std::string & content,
    const std::string & path)
{
    if (content.size() > 4 &&
        content.substr(0, 4) == "TES3")
    {
        try
        {
            size_t rec_beg = 0;
            size_t rec_size = 0;
            size_t rec_end = 0;
            while (rec_end != content.size())
            {
                rec_beg = rec_end;
                rec_size = Tools::convertStringByteArrayToUInt(content.substr(rec_beg + 4, 4)) + 16;
                rec_end = rec_beg + rec_size;
                records.push_back(content.substr(rec_beg, rec_size));
            }
            is_loaded = true;
        }
        catch (const std::exception & e)
        {
            Tools::addLog("--> Error parsing \"" + path + "\" (possibly broken file or record)!\r\n");
            Tools::addLog("--> Exception: " + std::string(e.what()) + "\r\n");
            is_loaded = false;
        }
    }
    else
    {
        Tools::addLog("--> Error parsing \"" + path + "\" (isn't TES3 plugin)!\r\n");
        is_loaded = false;
    }
}

//----------------------------------------------------------
void EsmReader::setName(const std::string & path)
{
    if (is_loaded)
    {
        name_full = path.substr(path.find_last_of("\\/") + 1);
        name_prefix = name_full.substr(0, name_full.find_last_of("."));
        name_suffix = name_full.substr(name_full.rfind("."));
    }
}

//----------------------------------------------------------
void EsmReader::setTime(const std::string & path)
{
    if (is_loaded)
    {
        time = boost::filesystem::last_write_time(path);
    }
}

//----------------------------------------------------------
void EsmReader::selectRecord(size_t i)
{
    if (is_loaded)
    {
        rec = &records[i];
        rec_size = rec->size();
        rec_id = rec->substr(0, 4);
    }
}

//----------------------------------------------------------
void EsmReader::replaceRecord(const std::string & new_rec)
{
    if (is_loaded)
    {
        *rec = new_rec;
    }
}

//----------------------------------------------------------
void EsmReader::setKey(const std::string & id)
{
    if (is_loaded)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        key.id = id;

        try
        {
            mainLoop(cur_pos, cur_size, cur_id, cur_text, key);
            ifEndOfRecordReached(cur_pos, key);
        }
        catch (const std::exception & e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::setValue(const std::string & id)
{
    if (is_loaded)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        value.id = id;
        value.counter = 0;

        try
        {
            mainLoop(cur_pos, cur_size, cur_id, cur_text, value);
            ifEndOfRecordReached(cur_pos, value);
        }
        catch (const std::exception & e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::setNextValue(const std::string & id)
{
    if (is_loaded && value.exist)
    {
        size_t cur_pos;
        size_t cur_size;
        std::string cur_id;
        std::string cur_text;
        value.id = id;

        cur_pos = value.pos;
        cur_size = Tools::convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        cur_pos += 8 + cur_size;
        value.counter++;

        try
        {
            mainLoop(cur_pos, cur_size, cur_id, cur_text, value);
            ifEndOfRecordReached(cur_pos, value);
        }
        catch (const std::exception & e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::mainLoop(
    std::size_t & cur_pos,
    std::size_t & cur_size,
    std::string & cur_id,
    std::string & cur_text,
    EsmReader::SubRecord & subrecord)
{
    while (cur_pos != rec->size())
    {
        cur_id = rec->substr(cur_pos, 4);
        cur_size = Tools::convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        if (cur_id == value.id)
        {
            cur_text = rec->substr(cur_pos + 8, cur_size);
            subrecord.content = cur_text;
            subrecord.text = Tools::eraseNullChars(subrecord.content);
            subrecord.pos = cur_pos;
            subrecord.size = cur_size;
            subrecord.exist = true;
            break;
        }
        cur_pos += 8 + cur_size;
    }
}

//----------------------------------------------------------
void EsmReader::ifEndOfRecordReached(
    std::size_t & cur_pos,
    EsmReader::SubRecord & subrecord)
{
    if (cur_pos == rec->size())
    {
        subrecord.content = "";
        subrecord.pos = cur_pos;
        subrecord.size = 0;
        subrecord.exist = false;
    }
}

//----------------------------------------------------------
void EsmReader::handleException(const std::exception & e)
{
    std::string cur_rec = Tools::replaceNonReadableCharsWithDot(*rec);
    Tools::addLog("--> Error in function (possibly broken record)!\r\n");
    Tools::addLog(cur_rec + "\r\n");
    Tools::addLog("--> Exception: " + std::string(e.what()) + "\r\n");
    is_loaded = false;
}

//----------------------------------------------------------
Tools::Encoding EsmReader::detectEncoding()
{
    for (size_t i = 0; i < getRecords().size(); ++i)
    {
        selectRecord(i);
        if (getRecordId() == "INFO")
            setValue("NAME");

        if (detectWindows1250Encoding(getValue().text))
        {
            Tools::addLog("--> Windows-1250 encoding detected!\r\n");
            Tools::addLog("INFO: " + getValue().text + "\r\n", true);
            return Tools::Encoding::WINDOWS_1250;
        }
    }
    return Tools::Encoding::UNKNOWN;
}

//----------------------------------------------------------
bool EsmReader::detectWindows1250Encoding(const std::string & val_text)
{
    // 156 œ ś
    // 159 Ÿ ź
    // 179 ³ ł
    // 185 ¹ ą
    // 191 ¿ ż
    // 230 æ ć
    // 234 ê ę
    // 241 ñ ń
    // 243 ó ó <- found in Tamriel Rebuilt

    std::ostringstream ss;
    ss
        << static_cast<char>(156)
        << static_cast<char>(159)
        << static_cast<char>(179)
        << static_cast<char>(185)
        << static_cast<char>(191)
        << static_cast<char>(230)
        << static_cast<char>(234)
        << static_cast<char>(241);

    return val_text.find_first_of(ss.str()) != std::string::npos;
}
