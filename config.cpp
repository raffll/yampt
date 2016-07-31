#include "config.hpp"

//----------------------------------------------------------
array<string, 4> config::sep = {"<br>", "<h3>", "</h3>", "<hr>"};

//----------------------------------------------------------
string config::converted_path = "";
string config::converted_suffix = "";

//----------------------------------------------------------
vector<string> config::key_message = {"messagebox", "say ", "say,", "choice"};
vector<string> config::key_dial = {"addtopic"};
vector<string> config::key_cell = {"positioncell", "getpccell", "aifollowcell",
				   "placeitemcell", "showmap"};

//----------------------------------------------------------
void config::readConfig()
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
void config::setConfigStatus(bool st)
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
void config::parseConfig()
{
	size_t pos;
	pos = content.find("CONVERTED_PATH");
	pos = content.find("=");



