#include "EsmTools.hpp"

using namespace std;

//----------------------------------------------------------
EsmTools::EsmTools()
{
	status = 0;
}

//----------------------------------------------------------
void EsmTools::readFile(string path)
{
	ifstream file(path, ios::binary);
	if(file)
	{
		string content;
		char buffer[16384];
		size_t size = file.tellg();
		content.reserve(size);
		streamsize chars_read;
		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			content.append(buffer, chars_read);
		}
		if(content.size() > 4 && content.substr(0, 4) == "TES3")
		{
			status = 1;
			setName(path);
			setRecColl(content);
		}
	}
	printStatus(path);
}

//----------------------------------------------------------
void EsmTools::printStatus(string path)
{
	if(status == 0)
	{
		Config::appendLog("--> Error while loading " + path +
				  " (wrong path or isn't TES3 plugin)!\r\n");
	}
	else
	{
		Config::appendLog("--> Loading " + path + "...\r\n");
	}
}

//----------------------------------------------------------
void EsmTools::setName(string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
	name_prefix = name.substr(0, name.find_last_of("."));
	name_suffix = name.substr(name.rfind("."));
}

//----------------------------------------------------------
void EsmTools::setRecColl(string &content)
{
	if(status == 1)
	{
		size_t rec_beg = 0;
		size_t rec_size = 0;
		size_t rec_end = 0;
		while(rec_end != content.size())
		{
			rec_beg = rec_end;
			rec_size = byteToInt(content.substr(rec_beg + 4, 4)) + 16;
			rec_end = rec_beg + rec_size;
			rec_coll.push_back(content.substr(rec_beg, rec_size));
		}
	}
}

//----------------------------------------------------------
unsigned int EsmTools::byteToInt(const string &str)
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
