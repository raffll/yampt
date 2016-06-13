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
void esmtools::setNextRec()
{
	if(esm_status == 1)
	{
		rec_beg = rec_end;
		rec_id = esm_content.substr(rec_beg, 4);
		rec_size = byteToInt(esm_content.substr(rec_beg + 4, 4)) + 16;
		rec_end = rec_beg + rec_size;
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
void esmtools::setPriSubRec(string id)
{
	if(esm_status == 1)
	{
		pri_id = id;
		pri_pos = rec_content.find(id);
		if(pri_id == "INDX")
		{
			int indx = byteToInt(rec_content.substr(pri_pos + 8, 4));
			ostringstream ss;
			ss << std::setfill('0') << std::setw(3) << indx;
			pri_text = ss.str();
		}
		else if(pri_id == "SCHD")
		{
			pri_size = 32;
			pri_text = rec_content.substr(pri_pos + 8, pri_size);
			for(unsigned i = 0; i < pri_size; i++)
			{
				if(pri_text.at(i) == '\0')
				{
					pri_text.resize(i);
					break;
				}
			}
		}
		else if(pri_pos != string::npos)
		{
			pri_size = byteToInt(rec_content.substr(pri_pos + 4, 4));
			pri_text = rec_content.substr(pri_pos + 8, pri_size);
			eraseNullChars(pri_text);
		}
		else
		{
			pri_text.erase();
		}
	}
}

//----------------------------------------------------------
void esmtools::setSecSubRec(string id)
{
	if(esm_status == 1)
	{
		sec_id = id;
		sec_pos = rec_content.find(id);
		if(sec_pos != string::npos)
		{
			sec_size = byteToInt(rec_content.substr(sec_pos + 4, 4));
			sec_text = rec_content.substr(sec_pos + 8, sec_size);
			eraseNullChars(sec_text);
		}
		else
		{
			sec_text.erase();
		}
	}
}

//----------------------------------------------------------
void esmtools::setColl(collkind e)
{
	//{"getpccell", "positioncell", "showmap"};
	//"addtopic
	if(esm_status == 1)
	{
		string line;
		smatch found;
		istringstream ss(sec_text);
		text_coll.clear();
		switch(e)
		{
			case 0: // RNAM
			{
				sec_id = "RNAM";
				sec_pos = rec_content.find(sec_id);
				while(sec_pos != string::npos)
				{
					sec_size = byteToInt(rec_content.substr(sec_pos + 4, 4));
					sec_text = rec_content.substr(sec_pos + 8, sec_size);
					eraseNullChars(sec_text);
					text_coll.push_back(make_pair(sec_text, sec_pos));
					sec_pos = rec_content.find(sec_id, sec_pos + 4);
				}
				break;
			}
			case 1: // SCPT
			{
				regex re(".*((say|messagebox).*\".*\")", icase);
				while(getline(ss, line))
				{
					regex_search(line, found, re);
					if(found[0] != "")
					{
						text_coll.push_back(make_pair(eraseNewLineChar(line), 1));
					}
					else
					{
						text_coll.push_back(make_pair(eraseNewLineChar(line), 0));
					}
				}
				break;
			}
			case 2: // SCPTMESSAGE
			{
				regex re(".*((say|messagebox).*\".*\")", icase);
				while(getline(ss, line))
				{
					regex_search(line, found, re);
					if(found[0] != "")
					{
						text_coll.push_back(make_pair(eraseNewLineChar(line), 1));
					}
				}
				break;
			}
			case 3: // BNAM
			{
				regex re(".*((choice).*\".*\")", icase);
				while(getline(ss, line))
				{
					regex_search(line, found, re);
					if(found[0] != "")
					{
						text_coll.push_back(make_pair(eraseNewLineChar(line), 1));
					}
					else
					{
						text_coll.push_back(make_pair(eraseNewLineChar(line), 0));
					}
				}
				break;
			}
			case 4: // BNAMMESSAGE
			{
				regex re(".*((choice).*\".*\")", icase);
				while(getline(ss, line))
				{
					regex_search(line, found, re);
					if(found[0] != "")
					{
						text_coll.push_back(make_pair(eraseNewLineChar(line), 1));
					}
				}
				break;
			}
		}
	}
}

//----------------------------------------------------------
bool esmtools::loopCheck()
{
	if(rec_end != esm_content.size())
	{
		return true;
	}
	else
	{
		return false;
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
