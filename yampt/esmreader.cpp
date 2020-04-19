﻿#include "esmreader.hpp"

//----------------------------------------------------------
EsmReader::EsmReader(const std::string & path)
{
    std::string content = Tools::readFile(path);
    splitFileIntoRecordColl(content, path);
    setName(path);
    setTime(path);
}

//----------------------------------------------------------
void EsmReader::splitFileIntoRecordColl(
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
                rec_coll.push_back(content.substr(rec_beg, rec_size));
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
void EsmReader::setRecordTo(size_t i)
{
    if (is_loaded)
    {
        rec = &rec_coll[i];
        rec_size = rec->size();
        rec_id = rec->substr(0, 4);
    }
}

//----------------------------------------------------------
void EsmReader::replaceRecordContent(const std::string & new_rec)
{
    if (is_loaded)
    {
        *rec = new_rec;
    }
}

//----------------------------------------------------------
void EsmReader::setUniqueTo(const std::string & id)
{
    if (is_loaded)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        unique_id = id;

        try
        {
            uniqueMainLoop(cur_pos, cur_size, cur_id, cur_text);
            uniqueIfEndOfRecordReached(cur_pos);
        }
        catch (const std::exception & e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::uniqueMainLoop(
    std::size_t & cur_pos,
    std::size_t & cur_size,
    std::string & cur_id,
    std::string & cur_text)
{
    while (cur_pos != rec->size())
    {
        cur_id = rec->substr(cur_pos, 4);
        cur_size = Tools::convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        if (cur_id == unique_id)
        {
            if (unique_id == "DATA" && rec_id == "DIAL")
            {
                caseForDialogType(cur_pos, cur_text);
            }
            else if (unique_id == "INDX")
            {
                caseForINDX(cur_pos, cur_text);
            }
            else
            {
                caseForDefault(cur_pos, cur_size, cur_text);
            }
            break;
        }
        cur_pos += 8 + cur_size;
    }
}

//----------------------------------------------------------
void EsmReader::caseForDialogType(
    std::size_t & cur_pos,
    std::string & cur_text)
{
    size_t type = Tools::convertStringByteArrayToUInt(rec->substr(cur_pos + 8, 1));
    cur_text = Tools::dialog_type[type];
    unique_text = cur_text;
    unique_exist = true;
}

//----------------------------------------------------------
void EsmReader::caseForINDX(
    std::size_t & cur_pos,
    std::string & cur_text)
{
    size_t indx = Tools::convertStringByteArrayToUInt(rec->substr(cur_pos + 8, 4));
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(3) << indx;
    cur_text = ss.str();
    unique_text = cur_text;
    unique_exist = true;
}

//----------------------------------------------------------
void EsmReader::caseForDefault(
    std::size_t & cur_pos,
    std::size_t & cur_size,
    std::string & cur_text)
{
    cur_text = rec->substr(cur_pos + 8, cur_size);
    unique_text = cur_text;
    unique_exist = true;
}

//----------------------------------------------------------
void EsmReader::uniqueIfEndOfRecordReached(std::size_t & cur_pos)
{
    if (cur_pos == rec->size())
    {
        unique_text = "Unique id " + unique_id + " not found!";
        unique_exist = false;
    }
}

//----------------------------------------------------------
void EsmReader::setFriendlyTo(const std::string & id)
{
    if (is_loaded)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        friendly_id = id;
        friendly_counter = 0;

        try
        {
            friendlyMainLoop(cur_pos, cur_size, cur_id, cur_text);
            friendlyIfEndOfRecordReached(cur_pos);
        }
        catch (const std::exception & e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::setNextFriendlyTo(const std::string & id)
{
    if (is_loaded && friendly_exist)
    {
        size_t cur_pos;
        size_t cur_size;
        std::string cur_id;
        std::string cur_text;
        friendly_id = id;

        cur_pos = friendly_pos;
        cur_size = Tools::convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        cur_pos += 8 + cur_size;
        friendly_counter++;

        try
        {
            friendlyMainLoop(cur_pos, cur_size, cur_id, cur_text);
            friendlyIfEndOfRecordReached(cur_pos);
        }
        catch (const std::exception & e)
        {
            handleException(e);
        }
    }
}

//----------------------------------------------------------
void EsmReader::friendlyMainLoop(
    std::size_t & cur_pos,
    std::size_t & cur_size,
    std::string & cur_id,
    std::string & cur_text)
{
    while (cur_pos != rec->size())
    {
        cur_id = rec->substr(cur_pos, 4);
        cur_size = Tools::convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
        if (cur_id == friendly_id)
        {
            cur_text = rec->substr(cur_pos + 8, cur_size);
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
void EsmReader::friendlyIfEndOfRecordReached(std::size_t & cur_pos)
{
    if (cur_pos == rec->size())
    {
        friendly_text = "Friendly id " + friendly_id + " not found!";
        friendly_pos = cur_pos;
        friendly_size = 0;
        friendly_exist = false;
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
        setRecordTo(i);
        if (getRecordId() == "INFO")
            setFriendlyTo("NAME");

        if (detectWindows1250Encoding(getFriendlyText()))
            return Tools::Encoding::WINDOWS_1250;
    }
    return Tools::Encoding::WINDOWS_1252;
}

//----------------------------------------------------------
bool EsmReader::detectWindows1250Encoding(const std::string & friendly_text)
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

    return friendly_text.find_first_of(ss.str()) != std::string::npos;
}
