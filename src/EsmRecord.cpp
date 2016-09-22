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
void EsmRecord::setPri(string id)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
		pri_coll.clear();
		pri_id = id;
		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));
			if(cur_id == pri_id)
			{
				cur_text = rec->substr(cur_pos + 8, cur_size);
				eraseNullChars(cur_text);
				if(!cur_text.empty())
				{
					pri_coll.push_back(make_tuple(cur_pos, cur_size, cur_text, true));
				}
				else
				{
					pri_coll.push_back(make_tuple(0, 0, "", false));
				}
				break;
			}
			cur_pos += 8 + cur_size;
			if(cur_pos == rec->size())
			{
				pri_coll.push_back(make_tuple(0, 0, "", false));
			}
		}
	}
}

//----------------------------------------------------------
void EsmRecord::setPriColl(string id)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
		pri_coll.clear();
		pri_id = id;
		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));
			if(cur_id == pri_id)
			{
				cur_text = rec->substr(cur_pos + 8, cur_size);
				eraseNullChars(cur_text);
				pri_coll.push_back(make_tuple(cur_pos, cur_size, cur_text, true));
			}
			cur_pos += 8 + cur_size;
		}
	}
}

//----------------------------------------------------------
void EsmRecord::setSec(string id)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
		sec_coll.clear();
		sec_id = id;
		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));
			if(cur_id == sec_id)
			{
				cur_text = rec->substr(cur_pos + 8, cur_size);
				if(Config::getReplaceBrokenChars() == true)
				{
					replaceBrokenChars(cur_text);
				}
				eraseNullChars(cur_text);
				sec_coll.push_back(make_tuple(cur_pos, cur_size, cur_text, true));
				break;
			}
			cur_pos += 8 + cur_size;
			if(cur_pos == rec->size())
			{
				sec_coll.push_back(make_tuple(0, 0, "", false));
			}
		}
	}
}

//----------------------------------------------------------
void EsmRecord::setSecColl(string id)
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
		sec_coll.clear();
		sec_id = id;
		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));
			if(cur_id == sec_id)
			{
				cur_text = rec->substr(cur_pos + 8, cur_size);
				eraseNullChars(cur_text);
				sec_coll.push_back(make_tuple(cur_pos, cur_size, cur_text, true));
			}
			cur_pos += 8 + cur_size;
		}
	}
}

//----------------------------------------------------------
void EsmRecord::setPriINDX()
{
	if(status == true)
	{
		size_t cur_pos = 16;
		size_t cur_size = 0;
		string cur_id;
		string cur_text;
		pri_coll.clear();
		pri_id = "INDX";
		while(cur_pos != rec->size())
		{
			cur_id = rec->substr(cur_pos, 4);
			cur_size = convertByteArrayToInt(rec->substr(cur_pos + 4, 4));
			if(cur_id == pri_id)
			{
				int indx = convertByteArrayToInt(rec->substr(cur_pos + 8, 4));
				ostringstream ss;
				ss << setfill('0') << setw(3) << indx;
				cur_text = ss.str();
				pri_coll.push_back(make_tuple(cur_pos, 0, cur_text, true));
				break;
			}
			cur_pos += 8 + cur_size;
			if(cur_pos == rec->size())
			{
				pri_coll.push_back(make_tuple(0, 0, "", false));
			}
		}
	}
}

//----------------------------------------------------------
void EsmRecord::setSecScptColl(string id)
{
	if(status == true)
	{
		string line;
		string line_lowercase;
		string type;
		size_t pos;
		string text;
		setSec(id);
		istringstream ss(getSecText());
		scpt_coll.clear();
		while(getline(ss, line))
		{
			type = "NOCHANGE";
			eraseCarriageReturnChar(line);
			line_lowercase = line;
			transform(line_lowercase.begin(), line_lowercase.end(),
				  line_lowercase.begin(), ::tolower);
			for(auto &elem : key_message)
			{
				if(type == "NOCHANGE")
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						type = id;
					}
				}
			}
			for(auto &elem : key_dial)
			{
				if(type == "NOCHANGE")
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						type = "DIAL";
						extractText(line, text, pos);
					}
				}
			}
			for(auto &elem : key_cell)
			{
				if(type == "NOCHANGE")
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						type = "CELL";
						extractText(line, text, pos);

					}
				}
			}
			scpt_coll.push_back(make_tuple(type, line, text, pos));
		}
		if(!scpt_coll.empty() &&
		   getSecText().size() > 1 &&
		   getSecText().substr(getSecText().size() - 2) == "\r\n")
		{
			get<1>(scpt_coll[scpt_coll.size() - 1]).append("\r\n");
		}
	}
}

//----------------------------------------------------------
void EsmRecord::setSecMessageColl(string id)
{
	if(status == true)
	{
		string line;
		string line_lowercase;
		string type;
		size_t pos;
		string text;
		setSec(id);
		istringstream ss(getSecText());
		scpt_coll.clear();
		while(getline(ss, line))
		{
			type = "NOCHANGE";
			eraseCarriageReturnChar(line);
			line_lowercase = line;
			transform(line_lowercase.begin(), line_lowercase.end(),
				  line_lowercase.begin(), ::tolower);
			for(auto &elem : key_message)
			{
				if(type == "NOCHANGE")
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						type = "MESSAGE";
						scpt_coll.push_back(make_tuple(type, line, text, pos));
					}
				}
			}
		}
	}
}

//----------------------------------------------------------
void EsmRecord::setSecDialType(string id)
{
	if(status == true)
	{
		setSec(id);
		static array<string, 5> type_coll = {"T", "V", "G", "P", "J"};
		int type = convertByteArrayToInt(rec->substr(getSecPos() + 8, 1));
		dial_type = type_coll[type];
	}
}

//----------------------------------------------------------
void EsmRecord::eraseNullChars(string &str)
{
	size_t is_null = str.find('\0');
	if(is_null != string::npos)
	{
		str.erase(is_null);
	}
}

//----------------------------------------------------------
void EsmRecord::replaceBrokenChars(string &str)
{
	for(size_t i = 0; i < str.size(); i++)
	{
		if(static_cast<int>(str[i]) == -127)
		{
			str[i] = '?';
		}
	}
}

//----------------------------------------------------------
string EsmRecord::eraseCarriageReturnChar(string &str)
{
	if(str.find('\r') != string::npos)
	{
		str.erase(str.size() - 1);
	}
	return str;
}

//----------------------------------------------------------
void EsmRecord::extractText(const string &line, string &text, size_t &pos)
{
	if(line.find("\"", pos) != string::npos)
	{
		regex re("\"(.*?)\"");
		smatch found;
		sregex_iterator next(line.begin(), line.end(), re);
		sregex_iterator end;
		while(next != end)
		{
			found = *next;
			text = found[1].str();
			pos = found.position(0);
			next++;
		}
	}
	else
	{
		size_t last_ws_pos;
		pos = line.find(" ", pos);
		pos = line.find_first_not_of(" ", pos);
		text = line.substr(pos);
		last_ws_pos = text.find_last_not_of(" \t");
		if(last_ws_pos != string::npos)
		{
			text.erase(last_ws_pos + 1);
		}
	}
}
