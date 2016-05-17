#include "esmtools.hpp"

using namespace std;

//----------------------------------------------------------
esmtools::esmtools()
{
	is_loaded = 0;
}

//----------------------------------------------------------
void esmtools::readFile(const char* path)
{
	file_name = path;
	ifstream file(file_name, ios::binary);
	cutFileName(file_name);

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
		printStatus();
	}
	else
	{
		is_loaded = 0;
		printStatus();
	}
}

//----------------------------------------------------------
void esmtools::printStatus()
{
	if(quiet == 0)
	{
		if(is_loaded == 1)
		{
			cout << file_name << " status: OK" << endl;
		}
		else
		{
			cout << file_name << " status: Error while loading file!" << endl;
		}
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
	if(is_loaded == 1)
	{
		rec_beg = rec_end;
		rec_id = file_content.substr(rec_beg, 4);
		rec_size = byteToInt(file_content.substr(rec_beg + 4, 4)) + 16;
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
			cutNullChar(pri_text);
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

		if(sec_id == "RNAM")
		{
			tmp_text.clear();

			while(sec_pos != string::npos)
			{
				sec_size = byteToInt(rec_content.substr(sec_pos + 4, 4));
				sec_text = rec_content.substr(sec_pos + 8, sec_size);
				cutNullChar(sec_text);
				tmp_text.push_back(sec_text);
				sec_pos = rec_content.find(id, sec_pos + 4);
			}
		}
		else if(sec_pos != string::npos)
		{
			sec_size = byteToInt(rec_content.substr(sec_pos + 4, 4));
			sec_text = rec_content.substr(sec_pos + 8, sec_size);
			cutNullChar(sec_text);
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
	int type = byteToInt(rec_content.substr(sec_pos + 8, 1));
	return type_coll[type];
}


