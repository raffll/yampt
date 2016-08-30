#include "Config.hpp"

using namespace std;

//----------------------------------------------------------
vector<string> Config::key_message = {"messagebox", "say ", "say,", "choice"};
vector<string> Config::key_dial = {"addtopic"};
vector<string> Config::key_cell = {"positioncell", "getpccell", "aifollowcell", "placeitemcell", "showmap"};

bool Config::allow_more_info;
string Config::output_suffix;
string Config::log;

//----------------------------------------------------------
Config::Config()
{
	status = 0;
	readConfig();
}

//----------------------------------------------------------
void Config::readConfig()
{
	ifstream file("yampt.cfg", ios::binary);
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
		if(!content.empty())
		{
			status = 1;
			parseConfig(content);
		}
	}
	printStatus();
}

//----------------------------------------------------------
void Config::printStatus()
{
	if(status == 0)
	{
		cout << "--> Error while loading yampt.cfg!\r\n";
	}
	else
	{
		cout << "--> Loading yampt.cfg...\r\n";
	}
}

//----------------------------------------------------------
void Config::appendLog(string message)
{
	log += message;
}

//----------------------------------------------------------
void Config::writeLog()
{
	if(!log.empty())
	{
		string name = "yampt.log";
		ofstream file(name, ios::binary);
		cout << "--> Writing " << name << "...\r\n";
		file << log;
	}
}

//----------------------------------------------------------
void Config::parseConfig(string &content)
{
	parseOutputSuffix(content);
	parseAllowMoreThan512InfoString(content);
}

//----------------------------------------------------------
void Config::parseOutputSuffix(string &content)
{
	regex re("(OUTPUT_SUFFIX=)\"(.*?)\"");
	smatch found;
	regex_search(content, found, re);
	output_suffix = found[2].str();
}

//----------------------------------------------------------
void Config::parseAllowMoreThan512InfoString(string &content)
{
	regex re("(ALLOW_MORE_THAN_512_INFO_STRING=)(.)");
	smatch found;
	regex_search(content, found, re);
	if(found[2].str() == "1")
	{
		allow_more_info = 1;
	}
}
