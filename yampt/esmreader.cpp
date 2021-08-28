#include "esmreader.hpp"

//----------------------------------------------------------
EsmReader::EsmReader(const std::string & path)
{
    std::string content = Tools::readFile(path);

    if (!content.empty())
        splitFile(content, path);

    name.setName(path);
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

                const auto & cnt = content.substr(rec_beg, rec_size);
                const auto & size = cnt.size();
                const auto & id = cnt.substr(0, 4);
                records.push_back({ id, cnt, size, false });
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
        rec = &records.at(i);
        key = {};
        value = {};
    }
}

//----------------------------------------------------------
void EsmReader::replaceRecord(const std::string & content)
{
    if (is_loaded)
    {
        rec->content = content;
        rec->modified = true;
        rec->size = content.size();
    }
}

//----------------------------------------------------------
void EsmReader::setModified(size_t i)
{
    if (is_loaded)
    {
        records.at(i).modified = true;
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
        cur_size = Tools::convertStringByteArrayToUInt(rec->content.substr(cur_pos + 4, 4));
        cur_pos += 8 + cur_size;
        value.counter++;

        try
        {
            mainLoop(cur_pos, cur_size, cur_id, cur_text, value);
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
    while (cur_pos != rec->content.size())
    {
        cur_id = rec->content.substr(cur_pos, 4);
        cur_size = Tools::convertStringByteArrayToUInt(rec->content.substr(cur_pos + 4, 4));
        if (cur_id == subrecord.id)
        {
            cur_text = rec->content.substr(cur_pos + 8, cur_size);
            subrecord.content = cur_text;
            subrecord.text = Tools::eraseNullChars(subrecord.content);
            subrecord.pos = cur_pos;
            subrecord.size = cur_size;
            subrecord.exist = true;
            break;
        }
        cur_pos += 8 + cur_size;
    }

    if (cur_pos == rec->content.size())
    {
        subrecord.content = "N/A";
        subrecord.text = "N/A";
        subrecord.pos = cur_pos;
        subrecord.size = 0;
        subrecord.exist = false;
    }
}

//----------------------------------------------------------
void EsmReader::handleException(const std::exception & e)
{
    std::string cur_rec = Tools::replaceNonReadableCharsWithDot(rec->content);
    Tools::addLog("--> Error in function (possibly broken record)!\r\n");
    Tools::addLog(cur_rec + "\r\n");
    Tools::addLog("--> Exception: " + std::string(e.what()) + "\r\n");
    is_loaded = false;
}

//----------------------------------------------------------
size_t EsmReader::getModifiedCount()
{
    size_t count = 0;
    for (const auto & record : records)
    {
        if (record.modified)
            count++;
    }
    return count;
}
