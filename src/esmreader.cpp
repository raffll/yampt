#include "esmreader.hpp"

//----------------------------------------------------------
EsmReader::EsmReader()
{

}

//----------------------------------------------------------
void EsmReader::readFile(const std::string &path)
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
        setRecordColl(content, path);
    }
    else
    {
        std::cout << "--> Error while loading " + path + " (wrong path)!" << std::endl;
        status = false;
    }
}

//----------------------------------------------------------
void EsmReader::setName(const std::string &path)
{
    name_full = path.substr(path.find_last_of("\\/") + 1);
    name_prefix = name_full.substr(0, name_full.find_last_of("."));
    name_suffix = name_full.substr(name_full.rfind("."));
}

//----------------------------------------------------------
void EsmReader::setTime(const std::string &path)
{
    time = boost::filesystem::last_write_time(path);
}

//----------------------------------------------------------
void EsmReader::setRecordColl(const std::string &content,
                              const std::string &path)
{
    if(content.size() > 4 && content.substr(0, 4) == "TES3")
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
            setName(path);
            setTime(path);
            std::cout << "--> Loading " + path + "..." << std::endl;
            status = true;
        }
        catch(std::exception const& e)
        {
            std::cout << "--> Error loading " + path + "..." << std::endl;
            std::cout << "--> Error in function setRecColl() (possibly broken file or record)!" << std::endl;
            std::cout << "--> Exception: " << e.what() << std::endl;
            status = false;
        }
    }
    else
    {
        std::cout << "--> Error loading " + path + "..." << std::endl;
        std::cout << "--> Error in function setRecColl() (isn't TES3 plugin)!" << std::endl;
        status = false;
    }
}

//----------------------------------------------------------
void EsmReader::setRecordTo(size_t i)
{
    if(status == true)
    {
        rec = &rec_coll[i];
        rec_size = rec->size();
        rec_id = rec->substr(0, 4);
    }
}

//----------------------------------------------------------
void EsmReader::setNewRecordContent(const std::string &new_rec)
{
    if(status == true)
    {
        *rec = new_rec;
    }
}

//----------------------------------------------------------
void EsmReader::setUniqueTo(const std::string id,
                            const bool erase_null)
{
    if(status == true)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        unique_id = id;

        try
        {
            // Main search loop
            while(cur_pos != rec->size())
            {
                cur_id = rec->substr(cur_pos, 4);
                cur_size = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
                if(cur_id == unique_id)
                {
                    cur_text = rec->substr(cur_pos + 8, cur_size);
                    if(erase_null == true)
                    {
                        cur_text = tools.eraseNullChars(cur_text);
                    }

                    // Unique key cannot be empty
                    if(!cur_text.empty())
                    {
                        unique_text = cur_text;
                        unique_status = true;
                    }
                    else
                    {
                        unique_text = "Unique text is empty!";
                        unique_status = false;
                    }
                    break;
                }
                cur_pos += 8 + cur_size;
            }

            // If end of record reached
            if(cur_pos == rec->size())
            {
                unique_text = "Unique id " + unique_id + " not found!";
                unique_status = false;
            }
        }
        catch(std::exception const& e)
        {
            std::string cur_rec = tools.replaceNonReadableCharsWithDot(*rec);
            std::cout << "--> Error in function setUniqueTo() (possibly broken record)!" << std::endl;;
            std::cout << cur_rec << std::endl;
            std::cout << "--> Exception: " << e.what() << std::endl;
            status = false;
        }
    }
}

//----------------------------------------------------------
void EsmReader::setUniqueToINDX()
{
    if(status == true)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        unique_id = "INDX";

        try
        {
            // Main search loop
            while(cur_pos != rec->size())
            {
                cur_id = rec->substr(cur_pos, 4);
                cur_size = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
                if(cur_id == unique_id)
                {
                    int indx = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 8, 4));
                    std::ostringstream ss;
                    ss << std::setfill('0') << std::setw(3) << indx;
                    cur_text = ss.str();
                    unique_text = cur_text;
                    unique_status = true;
                    break;
                }
                cur_pos += 8 + cur_size;
            }

            // If end of record reached
            if(cur_pos == rec->size())
            {
                unique_text = "Unique id " + unique_id + " not found!";
                unique_status = false;
            }
        }
        catch(std::exception const& e)
        {
            std::string cur_rec = tools.replaceNonReadableCharsWithDot(*rec);
            std::cout << "--> Error in function setUniqueToINDX() (possibly broken record)!" << std::endl;;
            std::cout << cur_rec << std::endl;
            std::cout << "--> Exception: " << e.what() << std::endl;
            status = false;
        }
    }
}

//----------------------------------------------------------
void EsmReader::setUniqueToDialogType()
{
    if(status == true &&
       rec_id == "DIAL")
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        unique_id = "DATA";

        try
        {
            // Main search loop
            while(cur_pos != rec->size())
            {
                cur_id = rec->substr(cur_pos, 4);
                cur_size = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 4, 4));
                if(cur_id == unique_id)
                {
                    int type = tools.convertStringByteArrayToUInt(rec->substr(cur_pos + 8, 1));
                    cur_text = yampt::dialog_type[type];
                    unique_text = cur_text;
                    unique_status = true;
                    break;
                }
                cur_pos += 8 + cur_size;
            }

            // If end of record reached
            if(cur_pos == rec->size())
            {
                unique_text = "Unique id " + unique_id + " not found!";
                unique_status = false;
            }
        }
        catch(std::exception const& e)
        {
            std::string cur_rec = tools.replaceNonReadableCharsWithDot(*rec);
            std::cout << "--> Error in function setUniqueToDialogType() (possibly broken record)!" << std::endl;;
            std::cout << cur_rec << std::endl;
            std::cout << "--> Exception: " << e.what() << std::endl;
            status = false;
        }
    }
}

//----------------------------------------------------------
void EsmReader::setFirstFriendlyTo(const std::string &id,
                                   const bool erase_null)
{
    if(status == true)
    {
        size_t cur_pos = 16;
        size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        friendly_id = id;
        friendly_counter = 0;

        try
        {
            // Main search loop
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
                    friendly_status = true;
                    break;
                }
                cur_pos += 8 + cur_size;
            }

            // If end of record reached
            if(cur_pos == rec->size())
            {
                friendly_text = "Friendly id " + friendly_id + " not found!";
                friendly_pos = cur_pos;
                friendly_size = 0;
                friendly_status = false;
            }
        }
        catch(std::exception const& e)
        {
            std::string cur_rec = tools.replaceNonReadableCharsWithDot(*rec);
            std::cout << "--> Error in function setFirstFriendlyTo() (possibly broken record)!" << std::endl;
            std::cout << cur_rec << std::endl;
            std::cout << "--> Exception: " << e.what() << std::endl;
            status = false;
        }
    }
}

//----------------------------------------------------------
void EsmReader::setNextFriendlyTo(const std::string &id,
                                  const bool erase_null)
{
    if(status == true &&
       friendly_status == true)
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
            // Main search loop
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
                    friendly_status = true;
                    break;
                }
                cur_pos += 8 + cur_size;
            }

            // If end of record reached
            if(cur_pos == rec->size())
            {
                friendly_text = "Friendly id " + friendly_id + " not found!";
                friendly_pos = cur_pos;
                friendly_size = 0;
                friendly_status = false;
            }
        }
        catch(std::exception const& e)
        {
            std::string cur_rec = tools.replaceNonReadableCharsWithDot(*rec);
            std::cout << "--> Error in function setNextFriendly() (possibly broken record)!" << std::endl;
            std::cout << cur_rec << std::endl;
            std::cout << "--> Exception: " << e.what() << std::endl;
            status = false;
        }
    }
}
