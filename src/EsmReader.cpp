#include "EsmReader.hpp"

using namespace std;

//----------------------------------------------------------
EsmReader::EsmReader()
{

}

//----------------------------------------------------------
void EsmReader::readFile(string path)
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
			status = true;
			setName(path);
			setRecColl(content);
		}
	}
	printStatus(path);
}

//----------------------------------------------------------
void EsmReader::printStatus(string path)
{
	if(status == false)
	{
		cout << "--> Error while loading " + path +
			" (wrong path or isn't TES3 plugin)!\r\n";
	}
	else
	{
		cout << "--> Loading " + path + "...\r\n";
	}
}

//----------------------------------------------------------
void EsmReader::setName(string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
	name_prefix = name.substr(0, name.find_last_of("."));
	name_suffix = name.substr(name.rfind("."));
}

//----------------------------------------------------------
void EsmReader::setRecColl(string &content)
{
	if(status == true)
	{
		size_t rec_beg = 0;
		size_t rec_size = 0;
		size_t rec_end = 0;
		while(rec_end != content.size())
		{
			rec_beg = rec_end;
			rec_size = convertByteArrayToInt(content.substr(rec_beg + 4, 4)) + 16;
			rec_end = rec_beg + rec_size;
			rec_coll.push_back(content.substr(rec_beg, rec_size));
		}
	}
}

//----------------------------------------------------------
unsigned int EsmReader::convertByteArrayToInt(const string &str)
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
