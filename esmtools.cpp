#include "esmtools.hpp"

using namespace std;

//----------------------------------------------------------
Esmtools::Esmtools(const Esmtools& that) : esm_status(that.esm_status),
					   esm_name(that.esm_name),
					   esm_prefix(that.esm_prefix),
					   esm_suffix(that.esm_suffix),
					   esm_content(that.esm_content),
					   rec_beg(that.rec_beg),
					   rec_end(that.rec_end),
					   rec_size(that.rec_size),
					   rec_id(that.rec_id),
					   rec_content(that.rec_content),
					   pri_pos(that.pri_pos),
					   pri_size(that.pri_size),
					   pri_id(that.pri_id),
					   pri_text(that.pri_text),
					   sec_pos(that.sec_pos),
					   sec_size(that.sec_size),
					   sec_id(that.sec_id),
					   sec_text(that.sec_text),
					   text_coll(that.text_coll)
{

}

//----------------------------------------------------------
Esmtools& Esmtools::operator=(const Esmtools& that)
{
	esm_status = that.esm_status;
	esm_name = that.esm_name;
	esm_prefix = that.esm_prefix;
	esm_suffix = that.esm_suffix;
	esm_content = that.esm_content;
	rec_beg = that.rec_beg;
	rec_end = that.rec_end;
	rec_size = that.rec_size;
	rec_id = that.rec_id;
	rec_content = that.rec_content;
	pri_pos = that.pri_pos;
	pri_size = that.pri_size;
	pri_id = that.pri_id;
	pri_text = that.pri_text;
	sec_pos = that.sec_pos;
	sec_size = that.sec_size;
	sec_id = that.sec_id;
	sec_text = that.sec_text;
	text_coll = that.text_coll;
	return *this;
}

//----------------------------------------------------------
Esmtools::~Esmtools()
{

}

//----------------------------------------------------------
void Esmtools::readEsm(string path)
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
		if(esm_content.size() > 4 && esm_content.substr(0, 4) == "TES3")
		{
			setEsmName(path);
			setEsmStatus(1, path);
		}
		else
		{
			setEsmStatus(0, path);
		}
	}
	else
	{
		setEsmStatus(0, path);
	}
}

//----------------------------------------------------------
void Esmtools::setEsmStatus(bool st, string path)
{
	if(st == 0)
	{
		cerr << "--> Error while loading " << path << " (wrong path or isn't TES3 plugin)!" << endl;
		esm_status = 0;
	}
	else
	{
		cerr << "--> Loading " << path << "..." << endl;
		esm_status = 1;
	}
}

//----------------------------------------------------------
void Esmtools::setEsmName(string path)
{
	esm_name = path.substr(path.find_last_of("\\/") + 1);
	esm_prefix = esm_name.substr(0, esm_name.find_last_of("."));
	esm_suffix = esm_name.substr(esm_name.rfind("."));
}

//----------------------------------------------------------
void Esmtools::resetRec()
{
	rec_beg = 0;
	rec_end = 0;
}

//----------------------------------------------------------
bool Esmtools::setNextRec()
{
	if(esm_status == 1)
	{
		rec_beg = rec_end;
		rec_id = esm_content.substr(rec_beg, 4);
		rec_size = byteToInt(esm_content.substr(rec_beg + 4, 4)) + 16;
		rec_end = rec_beg + rec_size;
		if(rec_end != esm_content.size())
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 1;
	}
}

//----------------------------------------------------------
void Esmtools::setRecContent()
{
	if(esm_status == 1)
	{
		rec_content = esm_content.substr(rec_beg, rec_size);
		pri_pos = 0;
		sec_pos = 0;
	}
}

//----------------------------------------------------------
bool Esmtools::setPriSubRec(string id, size_t next)
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
		return 1;
	}
}

//----------------------------------------------------------
bool Esmtools::setSecSubRec(string id, size_t next)
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
		return 1;
	}
}

//----------------------------------------------------------
void Esmtools::setPriSubRecINDX()
{
	if(esm_status == 1)
	{
		pri_id = "INDX";
		pri_pos = rec_content.find(pri_id);
		int indx = byteToInt(rec_content.substr(pri_pos + 8, 4));
		ostringstream ss;
		ss << setfill('0') << setw(3) << indx;
		pri_size = 0;
		pri_text = ss.str();
	}
}

//----------------------------------------------------------
void Esmtools::setCollScript()
{
	if(esm_status == 1)
	{
		string line;
		string line_lowercase;
		string type;
		size_t pos;
		string text;
		istringstream ss(sec_text);
		text_coll.clear();
		while(getline(ss, line))
		{
			type = "NOCHANGE";
			eraseCarriageReturnChar(line);
			line_lowercase = line;
			transform(line_lowercase.begin(), line_lowercase.end(),
				  line_lowercase.begin(), ::tolower);
			for(auto &elem : Config::key_message)
			{
				if(type == "NOCHANGE")
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						type = "MESSAGE";
					}
				}
			}
			for(auto &elem : Config::key_dial)
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
			for(auto &elem : Config::key_cell)
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
			text_coll.push_back(make_tuple(type, line, text, pos));
		}
		addEndLineToLastCollItem();
	}
}

//----------------------------------------------------------
void Esmtools::setCollMessageOnly()
{
	if(esm_status == 1)
	{
		string line;
		string line_lowercase;
		string type;
		size_t pos;
		string text;
		istringstream ss(sec_text);
		text_coll.clear();
		while(getline(ss, line))
		{
			type = "NOCHANGE";
			eraseCarriageReturnChar(line);
			line_lowercase = line;
			transform(line_lowercase.begin(), line_lowercase.end(),
				  line_lowercase.begin(), ::tolower);
			for(auto &elem : Config::key_message)
			{
				if(type == "NOCHANGE")
				{
					pos = line_lowercase.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						type = "MESSAGE";
						text_coll.push_back(make_tuple(type, line, text, pos));
					}
				}
			}
		}
	}
}

//----------------------------------------------------------
string Esmtools::dialType()
{
	static array<string, 5> type_coll = {"T", "V", "G", "P", "J"};
	int type = byteToInt(rec_content.substr(sec_pos + 8, 1));
	return type_coll[type];
}

//----------------------------------------------------------
unsigned int Esmtools::byteToInt(const string &str)
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
void Esmtools::eraseNullChars(string &str)
{
	size_t is_null = str.find('\0');
	if(is_null != string::npos)
	{
		str.erase(is_null);
	}
}

//----------------------------------------------------------
string Esmtools::eraseCarriageReturnChar(string &str)
{
	if(str.find('\r') != string::npos)
	{
		str.erase(str.size() - 1);
	}
	return str;
}

//----------------------------------------------------------
void Esmtools::addEndLineToLastCollItem()
{
	if(!text_coll.empty() && sec_text.size() > 1 && sec_text.substr(sec_text.size() - 2) == "\r\n")
	{
		get<1>(text_coll[text_coll.size() - 1]).append("\r\n");
	}
}

//----------------------------------------------------------
/*void Esmtools::extractText(const string &line, string &text, size_t &pos)
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
	//cout << line << endl;
	//cout << text << " " << pos << endl;
}*/

//----------------------------------------------------------
void Esmtools::extractText(const string &line, string &text, size_t &pos)
{
	size_t pos_end;
	pos = line.find("\"", pos);
	if(pos != string::npos)
	{
		pos_end = line.find("\"", pos + 1) + 1;
		text = line.substr(pos, pos_end - pos);
		text = text.substr(1, text.size() - 2);
	}
	else
	{
		pos = line.find(" ") + 1;
		text = line.substr(pos);
	}
}
