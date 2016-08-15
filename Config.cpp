#include "Config.hpp"

using namespace std;

//----------------------------------------------------------
vector<string> Config::sep = {"^", "<h3>", "</h3>", "<hr>"};
string Config::sep_line = "<!-------------------------------------------------------------->\r\n";
string Config::base_dictionary_path = "";
string Config::output_path = "";
string Config::output_suffix = "";
string Config::log = "";
bool Config::status = 0;
vector<string> Config::key_message = {"messagebox", "say ", "say,", "choice"};
vector<string> Config::key_dial = {"addtopic"};
vector<string> Config::key_cell = {"positioncell", "getpccell", "aifollowcell",
				   "placeitemcell", "showmap"};

//----------------------------------------------------------
Config::Config()
{
	readConfig();
}

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
			parseOutputPath();
			parseOutputSuffix();
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
		appendLog("--> Error while loading yampt.cfg!\r\n");
		status = 0;
	}
	else
	{
		appendLog("--> Loading yampt.cfg...\r\n");
		status = 1;
	}
}

//----------------------------------------------------------
void Config::appendLog(string message, bool no_standard_output)
{
	if(no_standard_output == 0)
	{
		cout << message;
		log += message;
	}
	else
	{
		log += message;
	}
}

//----------------------------------------------------------
void Config::writeLog()
{
	string name = "yampt.log";
	ofstream file(output_path + name, ios::binary);
	appendLog("--> Writing " + name + "...\r\n");
	file << log;
}

//----------------------------------------------------------
void Config::parseOutputPath()
{
	regex re("(OUTPUT_PATH=)\"(.*?)\"");
	smatch found;
	regex_search(content, found, re);
	output_path = found[2].str();
}

//----------------------------------------------------------
void Config::parseOutputSuffix()
{
	regex re("(OUTPUT_SUFFIX=)\"(.*?)\"");
	smatch found;
	regex_search(content, found, re);
	output_suffix = found[2].str();
}
