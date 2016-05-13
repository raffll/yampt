#include "esmtools.hpp"

using namespace std;

//----------------------------------------------------------
esmtools::esmtools()
{
	is_loaded = 0;
}

//----------------------------------------------------------
esmtools::esmtools(const char* path)
{
	file_name = path;
	ifstream file(file_name, ios::binary);
	setFileName(file_name);

	if(file.good())
	{
		char buffer[16384];
		size_t file_size = file.tellg();
		file_content.reserve(file_size);
		streamsize chars_read;

		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			file_content.append(buffer, chars_read);
		}
		is_loaded = 1;
	}
	else
	{
		is_loaded = 0;
	}
}

//----------------------------------------------------------
void esmtools::printStatus()
{
	cout << file_name << " is loaded: " << is_loaded << " with size: " << file_content.size() << endl;
}

//----------------------------------------------------------
// set
//----------------------------------------------------------
void esmtools::setNextRec()
{
	if(is_loaded == 1)
	{
		rec_beg = rec_end;
		rec_id = file_content.substr(rec_beg, 4);
		rec_size = byteToInt(rec_beg + 4, &file_content) + 16;
		rec_end = rec_beg + rec_size;
	}
}

//----------------------------------------------------------
void esmtools::setRecContent()
{
	if(is_loaded == 1)
	{
		rec_content = file_content.substr(rec_beg, rec_size);
		pri_pos = 0;
		sec_pos = 0;
	}
}

//----------------------------------------------------------
void esmtools::setPriSubRec(const char* id)
{
	if(is_loaded == 1)
	{
		pri_id = id;
		pri_pos = rec_content.find(id);

		if(pri_id == "INDX")
		{
			int indx = byteToInt(pri_pos + 8, &rec_content);
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
			pri_size = byteToInt(pri_pos + 4, &rec_content);
			pri_text = rec_content.substr(pri_pos + 8, pri_size);
			cutText(&pri_text);
		}
		else
		{
			pri_text = "";
		}
	}
}

//----------------------------------------------------------
void esmtools::setSecSubRec(const char* id)
{
	if(is_loaded == 1)
	{
		sec_id = id;
		sec_pos = rec_content.find(id);

		string line;
		istringstream ss(sec_text);
		temp_text.clear();

		if(sec_id == "RNAM")
		{
			while(sec_pos != string::npos)
			{
				sec_size = byteToInt(sec_pos + 4, &rec_content);
				sec_text = rec_content.substr(sec_pos + 8, sec_size);
				cutText(&sec_text);
				temp_text.push_back(sec_text);
				sec_pos = rec_content.find(id, sec_pos + 4);
			}
		}
		else if(sec_pos != string::npos)
		{
			sec_size = byteToInt(sec_pos + 4, &rec_content);
			sec_text = rec_content.substr(sec_pos + 8, sec_size);
			cutText(&sec_text);
		}
		else
		{
			sec_text = "";
		}

		if(sec_id == "SCTX" || sec_id == "BNAM")
		{
			while(getline(ss, line))
			{
				if(line.find('\r') != string::npos)
				{
					line.erase(line.size() - 1);
				}
				temp_text.push_back(line);
			}
		}
	}
}

//----------------------------------------------------------
// get
//----------------------------------------------------------
string esmtools::getRecId()
{
	return rec_id;
}

//----------------------------------------------------------
string esmtools::getPriId()
{
	return pri_id;
}

//----------------------------------------------------------
string esmtools::getSecId()
{
	return sec_id;
}

//----------------------------------------------------------
string esmtools::getPriText()
{
	return pri_text;
}

//----------------------------------------------------------
string esmtools::getSecText()
{
	return sec_text;
}

//----------------------------------------------------------
bool esmtools::getStatus()
{
	return is_loaded;
}

//----------------------------------------------------------
// other
//----------------------------------------------------------
void esmtools::resetRec()
{
	rec_beg = 0;
	rec_end = 0;
}

//----------------------------------------------------------
bool esmtools::loopCheck()
{
	if(rec_end != file_content.size())
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
	int type = byteToInt(sec_pos + 8, &rec_content, 1);
	return type_coll[type];
}

//----------------------------------------------------------
void esmtools::cutText(string *str)
{
	size_t is_null = str->find('\0');
	while(is_null != string::npos)
	{
		str->erase(is_null, 1);
		is_null = str->find('\0');
	}
}

//----------------------------------------------------------
unsigned int esmtools::byteToInt(size_t pos, string *str, bool q)
{
	string bytes;
	char buffer[4];
	unsigned char ubuffer[4];
	unsigned int number;

	bytes = str->substr(pos, 4);
	bytes.copy(buffer, 4);
	for(int i = 0; i < 4; i++)
	{
		ubuffer[i] = buffer[i];
	}

	if(q == 0)
	{
		number = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
	}
	else
	{
		number = ubuffer[0];
	}
	return number;
}
