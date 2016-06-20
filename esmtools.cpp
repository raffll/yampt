#include "esmtools.hpp"

using namespace std;
using namespace std::regex_constants;

//----------------------------------------------------------
void esmtools::readEsm(string path)
{
	ifstream file(path, ios::binary);
	if(file)
	{
		char buffer[16384];
		size_t size = file.tellg();
		esm_content.reserve(size);
		streamsize chars_read;
		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			esm_content.append(buffer, chars_read);
		}
		if(!esm_content.empty() && esm_content.substr(0, 4) == "TES3")
		{
			setEsmName(path);
			setEsmStatus(1);
		}
		else
		{
			setEsmStatus(0);
		}
	}
	else
	{
		setEsmStatus(0);
	}
}

//----------------------------------------------------------
void esmtools::setEsmStatus(bool st)
{
	if(st == 0)
	{
		cerr << "Error while loading file! (wrong path or isn't TES3 plugin)" << endl;
		esm_status = 0;
	}
	else
	{
		cerr << "Loading " << esm_name << "..." << endl;
		esm_status = 1;
	}
}

//----------------------------------------------------------
void esmtools::setEsmName(string path)
{
	esm_name = path.substr(path.find_last_of("\\/") + 1);
	esm_prefix = esm_name.substr(0, esm_name.find_last_of("."));
	esm_suffix = esm_name.substr(esm_name.rfind("."));
}

//----------------------------------------------------------
void esmtools::resetRec()
{
	rec_beg = 0;
	rec_end = 0;
}

//----------------------------------------------------------
bool esmtools::setNextRec()
{
	if(esm_status == 1)
	{
		rec_beg = rec_end;
		rec_id = esm_content.substr(rec_beg, 4);
		rec_size = byteToInt(esm_content.substr(rec_beg + 4, 4)) + 16;
		rec_end = rec_beg + rec_size;
		if(rec_end != esm_content.size())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------------
void esmtools::setRecContent()
{
	if(esm_status == 1)
	{
		rec_content = esm_content.substr(rec_beg, rec_size);
		pri_pos = 0;
		sec_pos = 0;
	}
}

//----------------------------------------------------------
bool esmtools::setPriSubRec(string id, size_t next)
{
	if(esm_status == 1)
	{
		pri_id = id;
		pri_pos = rec_content.find(id, pri_pos + next);
		if(pri_pos != string::npos)
		{
			pri_size = byteToInt(rec_content.substr(pri_pos + 4, 4));
			pri_text = rec_content.substr(pri_pos + 8, pri_size);
			eraseNullChars(pri_text);
			return 1;
		}
		else
		{
			pri_size = 0;
			pri_text.erase();
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//----------------------------------------------------------
bool esmtools::setSecSubRec(string id, size_t next)
{
	if(esm_status == 1)
	{
		sec_id = id;
		sec_pos = rec_content.find(id, sec_pos + next);
		if(sec_pos != string::npos)
		{
			sec_size = byteToInt(rec_content.substr(sec_pos + 4, 4));
			sec_text = rec_content.substr(sec_pos + 8, sec_size);
			eraseNullChars(sec_text);
			return 1;
		}
		else
		{
			sec_size = 0;
			sec_text.erase();
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//----------------------------------------------------------
void esmtools::setPriSubRecINDX()
{
	if(esm_status == 1)
	{
		pri_id = "INDX";
		pri_pos = rec_content.find(pri_id);
		int indx = byteToInt(rec_content.substr(pri_pos + 8, 4));
		ostringstream ss;
		ss << std::setfill('0') << std::setw(3) << indx;
		pri_size = 0;
		pri_text = ss.str();
	}
}

//----------------------------------------------------------
void esmtools::setCollScript()
{
	if(esm_status == 1)
	{
		static vector<string> key_message = {"messagebox", "say ", "say,", "choice"};
		static vector<string> key_noid = {"addtopic", "positioncell", "getpccell", "aifollowcell",
						  "placeitemcell", "showmap"};
		string line;
		string line_lowercase;
		linekind found;
		size_t pos;
		size_t pos_end;
		string text;
		istringstream ss(sec_text);
		text_coll.clear();
		while(getline(ss, line))
		{
			found = NOCHANGE;
			eraseNewLineChar(line);
			line_lowercase = line;
			transform(line_lowercase.begin(), line_lowercase.end(),
				  line_lowercase.begin(), ::tolower);
			for(auto &elem : key_message)
			{
				if(found == NOCHANGE)
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						found = MESSAGE;
					}
				}
			}
			for(auto &elem : key_noid)
			{
				if(found == NOCHANGE)
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						if(elem == "addtopic")
						{
							found = DIAL;
						}
						else
						{
							found = CELL;
						}
						pos = line.find("\"", pos);
						if(pos != string::npos)
						{
							pos_end = line.find("\"", pos + 1) + 1;
							text = line.substr(pos, pos_end - pos);
						}
						else
						{
							pos = line.find(" ") + 1;
							text = line.substr(pos);
						}
					}
				}
			}
			if(found == MESSAGE)
			{
				text_coll.push_back(make_tuple(line, 0, MESSAGE, ""));
			}
			else if(found == DIAL)
			{
				text_coll.push_back(make_tuple(line, pos, DIAL, text));
			}
			else if(found == CELL)
			{
				text_coll.push_back(make_tuple(line, pos, CELL, text));
			}
			else
			{
				text_coll.push_back(make_tuple(line, 0, NOCHANGE, ""));
			}
		}
		addLastItemEndLine();
	}
}

//----------------------------------------------------------
void esmtools::setCollMessageOnly()
{
	if(esm_status == 1)
	{
		static vector<string> key_scpt = {"messagebox", "say ", "say,", "choice"};
		string line;
		string line_lowercase;
		linekind found;
		size_t pos;
		string text;
		istringstream ss(sec_text);
		text_coll.clear();
		while(getline(ss, line))
		{
			found = NOCHANGE;
			line_lowercase = line;
			transform(line_lowercase.begin(), line_lowercase.end(),
				  line_lowercase.begin(), ::tolower);
			for(auto &elem : key_scpt)
			{
				pos = line_lowercase.find(elem);
				if(pos != string::npos && line.rfind(";", pos) == string::npos)
				{
					found = MESSAGE;
				}
			}
			if(found == MESSAGE)
			{
				text_coll.push_back(make_tuple(eraseNewLineChar(line), 0, MESSAGE, ""));
			}
		}
	}
}

//----------------------------------------------------------
string esmtools::dialType()
{
	static array<string, 5> type_coll = {"T", "V", "G", "P", "J"};
	int type = byteToInt(rec_content.substr(sec_pos + 8, 1));
	return type_coll[type];
}

//----------------------------------------------------------
unsigned int esmtools::byteToInt(const string &str)
{
	char buffer[4];
	unsigned char ubuffer[4];
	unsigned int x;
	str.copy(buffer, 4);
	for(int i = 0; i < 4; i++)
	{
		ubuffer[i] = buffer[i];
	}
	if(str.size() == 4)
	{
		return x = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
	}
	else if(str.size() == 1)
	{
		return x = ubuffer[0];
	}
	else
	{
		return x = 0;
	}
}

//----------------------------------------------------------
void esmtools::eraseNullChars(string &str)
{
	size_t is_null = str.find('\0');
	if(is_null != string::npos)
	{
		str.erase(is_null);
	}
}

//----------------------------------------------------------
string esmtools::eraseNewLineChar(string &str)
{
	if(str.find('\r') != string::npos)
	{
		str.erase(str.size() - 1);
	}
	return str;
}

//----------------------------------------------------------
void esmtools::addLastItemEndLine()
{
	if(!text_coll.empty() && sec_text.size() > 1 && sec_text.substr(sec_text.size() - 2) == "\r\n")
	{
		get<0>(text_coll[text_coll.size() - 1]).append("\r\n");
	}
}
