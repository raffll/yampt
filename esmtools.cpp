#include "esmtools.hpp"

using namespace std;

//----------------------------------------------------------
void esmtools::readEsm(string path)
{
	ifstream file(path, ios::binary);
	setEsmName(path);

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

		if(esm_content.substr(0, 4) == "TES3")
		{
			setEsmStatus(loaded);
		}
		else
		{
			setEsmStatus(error);
		}
	}
	else
	{
		setEsmStatus(not_loaded);
	}
}

//----------------------------------------------------------
void esmtools::setEsmStatus(st e)
{
	switch(e)
	{
	case 0:
		cerr << "Loading " << esm_name << " status: Error while loading file!" << endl;
		esm_status = 0;
		break;

	case 1:
		cerr << "Loading " << esm_name << " status: OK" << endl;
		esm_status = 1;
		break;

	case 2:
		cerr << "Loading " << esm_name << " status: This isn't TES3 file!" << endl;
		esm_status = 0;
		esm_content.erase();
		break;
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
			cutNullChar(pri_text);
		}
		else
		{
			pri_text = "";
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
void esmtools::cutNullChar(string &str)
{
	size_t is_null = str.find('\0');
	while(is_null != string::npos)
	{
		str.erase(is_null, 1);
		is_null = str.find('\0');
	}
}
