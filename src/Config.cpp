#include "Config.hpp"

using namespace std;

//----------------------------------------------------------
vector<string> Config::key_message = {"messagebox", "say ", "say,", "choice"};
vector<string> Config::key_dial = {"addtopic"};
vector<string> Config::key_cell = {"positioncell", "getpccell", "aifollowcell", "placeitemcell", "showmap"};

string Config::output_path;
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
	string name = "yampt.log";
	ofstream file(output_path + name, ios::binary);
	cout << "--> Writing " << name << "...\r\n";
	file << log;
}

//----------------------------------------------------------
void Config::parseConfig(string &content)
{
	parseOutputPath(content);
	parseOutputSuffix(content);
}

//----------------------------------------------------------
void Config::parseOutputPath(string &content)
{
	regex re("(OUTPUT_PATH=)\"(.*?)\"");
	smatch found;
	regex_search(content, found, re);
	output_path = found[2].str();
}

//----------------------------------------------------------
void Config::parseOutputSuffix(string &content)
{
	regex re("(OUTPUT_SUFFIX=)\"(.*?)\"");
	smatch found;
	regex_search(content, found, re);
	output_suffix = found[2].str();
}
