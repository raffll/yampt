#include "EsmReader.hpp"

//----------------------------------------------------------
EsmReader::EsmReader()
{

}

//----------------------------------------------------------
void EsmReader::readFile(std::string path)
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
		if(content.size() > 4 && content.substr(0, 4) == "TES3")
		{
			status = true;
			setName(path);
			setRecColl(content);
		}
	}
	printStatus(path);
}

//----------------------------------------------------------
void EsmReader::printStatus(std::string path)
{
	if(status == false)
	{
        std::cout << "--> Error while loading " + path +
			" (wrong path or isn't TES3 plugin)!\r\n";
	}
	else
	{
        std::cout << "--> Loading " + path + "...\r\n";
	}
}

//----------------------------------------------------------
void EsmReader::setName(std::string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
	name_prefix = name.substr(0, name.find_last_of("."));
	name_suffix = name.substr(name.rfind("."));
}

//----------------------------------------------------------
void EsmReader::setRecColl(std::string &content)
{
	if(status == true)
	{
		size_t rec_beg = 0;
		size_t rec_size = 0;
		size_t rec_end = 0;
		while(rec_end != content.size())
		{
			rec_beg = rec_end;
			rec_size = convertByteArrayToInt(content.substr(rec_beg + 4, 4)) + 16;
			rec_end = rec_beg + rec_size;
			rec_coll.push_back(content.substr(rec_beg, rec_size));
		}
	}
}

//----------------------------------------------------------
void EsmReader::setRec(size_t i)
{
	if(status == true)
	{
		rec = &rec_coll[i];
		rec_size = rec->size();
		rec_id = rec->substr(0, 4);
	}
}

//----------------------------------------------------------
void EsmReader::setUnique(std::string id, bool erase_null)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
		unique_id = id;

		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));

			if(cur_id == unique_id)
			{
				if(id == "INDX")
				{
					int indx = convertByteArrayToInt(rec->substr(cur_pos + 8, 4));
                    std::ostringstream ss;
                    ss << std::setfill('0') << std::setw(3) << indx;
					cur_text = ss.str();
				}
				else if(rec_id == "DIAL" && id == "DATA")
				{
					int type = convertByteArrayToInt(rec->substr(cur_pos + 8, 1));
                    cur_text = yampt::dialog_type[type];
				}
				else
				{
					cur_text = rec->substr(cur_pos + 8, cur_size);
					if(erase_null == true)
					{
						eraseNullChars(cur_text);
					}
				}

				if(!cur_text.empty())
				{
					unique_text = cur_text;
					unique_status = true;
				}
				else
				{
					unique_text = "<EMPTY>";
					unique_status = false;
				}
				break;
			}
			cur_pos += 8 + cur_size;
			if(cur_pos == rec->size())
			{
				unique_text = "<NOTFOUND>";
				unique_status = false;
			}
		}
	}
}

//----------------------------------------------------------
bool EsmReader::setFriendly(std::string id, bool erase_null, bool next)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
		friendly_id = id;

		if(next == true && friendly_status == true)
		{
			cur_pos = friendly_pos;
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));
			cur_pos += 8 + cur_size;
			friendly_counter++;
		}
		else
		{
			friendly_counter = 0;
		}

		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));
			if(cur_id == friendly_id)
			{
				cur_text = rec->substr(cur_pos + 8, cur_size);
				if(erase_null == true)
				{
					eraseNullChars(cur_text);
				}
				friendly_text = cur_text;
				friendly_pos = cur_pos;
				friendly_size = cur_size;
				friendly_status = true;
				return true;
			}
			cur_pos += 8 + cur_size;
		}

		if(cur_pos == rec->size())
		{
			friendly_text = "<NOTFOUND>";
			friendly_pos = cur_pos;
			friendly_size = 0;
			friendly_status = false;
			return false;
		}
	}
	return false;
}

//----------------------------------------------------------
void EsmReader::setDump()
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
        std::string cur_id;
        std::string cur_text;
        std::string cur_dump;
		dump.erase();

		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));

			cur_text = rec->substr(cur_pos + 8, cur_size);
            cur_dump = "    " + cur_id + " " + std::to_string(cur_size) + " " + cur_text;
			for(size_t i = 0; i < cur_dump.size(); ++i)
			{
				if(isprint(cur_dump[i]))
				{
					dump += cur_dump[i];
				}
				else
				{
					dump += ".";
				}
			}
			dump += "\r\n";
			cur_pos += 8 + cur_size;
		}
	}
}
