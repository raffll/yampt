#include "esmtools.hpp"

using namespace std;

//----------------------------------------------------------
void esmtools::readFile(const char* path)
{
	ifstream file(path, ios::binary);
	name = path;
	name = name.substr(name.find_last_of("\\/") + 1);

	if(file)
	{
		char buffer[16384];
		size_t size = file.tellg();
		content.reserve(size);
		streamsize chars_read;

		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			content.append(buffer, chars_read);
		}

		if(content.substr(0, 4) == "TES3")
		{
			setStatus(loaded);
		}
		else
		{
			setStatus(error);
		}
	}
	else
	{
		setStatus(not_loaded);
	}
}

//----------------------------------------------------------
void esmtools::setStatus(st e)
{
	switch(e)
	{
	case 0:
		cerr << "--> Loading " << name << " status: Error while loading file!" << endl;
		status = 0;
		break;

	case 1:
		cerr << "--> Loading " << name << " status: OK" << endl;
		status = 1;
		break;

	case 2:
		cerr << "--> Loading " << name << " status: This isn't TES3 file!" << endl;
		status = 0;
		content.erase();
		break;
	}
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
	if(status == 1)
	{
		rec_beg = rec_end;
		rec_id = content.substr(rec_beg, 4);
		rec_size = byteToInt(content.substr(rec_beg + 4, 4)) + 16;
		rec_end = rec_beg + rec_size;
	}
}

//----------------------------------------------------------
void esmtools::setRecContent()
{
	if(status == 1)
	{
		rec_content = content.substr(rec_beg, rec_size);
		pri_pos = 0;
		sec_pos = 0;
	}
}

//----------------------------------------------------------
void esmtools::setPriSubRec(const char* id)
{
	if(status == 1)
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
			cutNullCharFromText(pri_text);
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
	if(status == 1)
	{
		sec_id = id;
		sec_pos = rec_content.find(id);

		if(sec_id == "RNAM")
		{
			tmp_text.clear();

			while(sec_pos != string::npos)
			{
				sec_size = byteToInt(rec_content.substr(sec_pos + 4, 4));
				sec_text = rec_content.substr(sec_pos + 8, sec_size);
				cutNullCharFromText(sec_text);
				tmp_text.push_back(sec_text);
				sec_pos = rec_content.find(id, sec_pos + 4);
			}
		}
		else if(sec_pos != string::npos)
		{
			sec_size = byteToInt(rec_content.substr(sec_pos + 4, 4));
			sec_text = rec_content.substr(sec_pos + 8, sec_size);
			cutNullCharFromText(sec_text);
		}
		else
		{
			sec_text = "";
		}

		if(sec_id == "SCTX" || sec_id == "BNAM")
		{
			string line;
			istringstream ss(sec_text);
			tmp_text.clear();

			while(getline(ss, line))
			{
				if(line.find('\r') != string::npos)
				{
					line.erase(line.size() - 1);
				}
				tmp_text.push_back(line);
			}
		}
	}
}

//----------------------------------------------------------
bool esmtools::loopCheck()
{
	if(rec_end != content.size())
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
	int type = byteToInt(rec_content.substr(sec_pos + 8, 1));
	return type_coll[type];
}

//----------------------------------------------------------
unsigned int esmtools::byteToInt(const string &str)
{
	char buffer[4];
	unsigned char ubuffer[4];
	unsigned int number;

	str.copy(buffer, 4);

	for(int i = 0; i < 4; i++)
	{
		ubuffer[i] = buffer[i];
	}

	if(str.size() == 4)
	{
		return number = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
	}
	else if(str.size() == 1)
	{
		return number = ubuffer[0];
	}
	else
	{
		return number = 0;
	}
}

//----------------------------------------------------------
void esmtools::cutNullCharFromText(string &str)
{
	size_t is_null = str.find('\0');
	while(is_null != string::npos)
	{
		str.erase(is_null, 1);
		is_null = str.find('\0');
	}
}
