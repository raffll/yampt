#include "config.hpp"

//----------------------------------------------------------
array<string, 4> Config::sep = {"<br>", "<h3>", "</h3>", "<hr>"};

//----------------------------------------------------------
string Config::converted_path = "";
string Config::converted_suffix = "";

//----------------------------------------------------------
vector<string> Config::key_message = {"messagebox", "say ", "say,", "choice"};
vector<string> Config::key_dial = {"addtopic"};
vector<string> Config::key_cell = {"positioncell", "getpccell", "aifollowcell",
				   "placeitemcell", "showmap"};

//----------------------------------------------------------
void Config::readConfig()
{
	ifstream file("yampt.cfg", ios::binary);
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
		if(!content.empty())
		{
			setConfigStatus(1);
		}
		else
		{
			setConfigStatus(0);
		}
	}
	else
	{
		setConfigStatus(0);
	}
}

//----------------------------------------------------------
void Config::setConfigStatus(bool st)
{
	if(st == 0)
	{
		cerr << "--> Error while loading yampt.cfg!" << endl;
		status = 0;
	}
	else
	{
		cerr << "--> Loading yampt.cfg..." << endl;
		status = 1;
	}
}

//----------------------------------------------------------
void Config::parseConfig()
{
	size_t pos;
	pos = content.find("CONVERTED_PATH");
	pos = content.find("=");
}
