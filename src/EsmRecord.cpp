#include "EsmRecord.hpp"

using namespace std;

//----------------------------------------------------------
EsmRecord::EsmRecord() : EsmReader()
{

}

//----------------------------------------------------------
void EsmRecord::setRec(size_t i)
{
	if(status == true)
	{
		rec = &rec_coll[i];
		rec_size = rec->size();
		rec_id = rec->substr(0, 4);
	}
}

//----------------------------------------------------------
void EsmRecord::setUnique(string id, bool erase_null)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
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
					ostringstream ss;
					ss << setfill('0') << setw(3) << indx;
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
					unique_text = "<Empty>";
					unique_status = false;
				}
				break;
			}
			cur_pos += 8 + cur_size;
			if(cur_pos == rec->size())
			{
				unique_text = "<NotFound>";
				unique_status = false;
			}
		}
	}
}

//----------------------------------------------------------
bool EsmRecord::setFriendly(string id, bool next, bool erase_null)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
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
			friendly_text = "<NotFound>";
			friendly_pos = cur_pos;
			friendly_size = 0;
			friendly_status = false;
			return false;
		}
	}
	return false;
}

//----------------------------------------------------------
void EsmRecord::setDump()
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
		string cur_dump;
		dump.erase();

		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));

			cur_text = rec->substr(cur_pos + 8, cur_size);
			cur_dump = "    " + cur_id + " " + to_string(cur_size) + " " + cur_text;
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
