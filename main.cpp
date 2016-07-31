#include "ui.hpp"

int main(int argc, char *argv[])
{
	std::vector<std::string> arg;
	for(int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	std::string usage = "Usage: " + arg[0] + " [command]"
		       "\n"
		       "\n  --help                               Print this message."
		       "\n  --make-all  [file1] [dict1]          Make dictionary from esp/esm plugin with all records."
		       "\n  --make-not  [file1] [dict1]          Make without records from dictionary."
		       "\n  --make-raw  [file1]                  Like \"--make-all\", but without DIAL translation."
		       "\n  --make-base [file1] [file2]          Make base dictionary from two different localized esm files."
		       "\n  --compare   [dict1] [dict2]          Compare two dictionaries and create differences log."
		       "\n  --merge     [dict1] <dict2> <dict3>  Validate, merge, sort and delete doubled records."
		       "\n  --convert   [file1] [dict1] <dict2>  Convert plugin from dictionaries in paths"
		       "\n                                       and create dictionary as \"--make-not\" command."
		       "\n  --scripts   [file1] <file2>          Write scripts content log."
		       "\n  --binary    [file1]                  Write binary log.";
	Config config;
	config.readConfig();
	Ui ui(arg);
	if(arg[1] == "--help")
	{
		std::cout << usage << endl;
	}
	else if(arg[1] == "--make-raw" && arg.size() == 3)
	{
		ui.makeDictRaw();
	}
	else if(arg[1] == "--make-base" && arg.size() == 4)
	{
		ui.makeDictBase();
	}
	else if(arg[1] == "--make-all")
	{
		ui.makeDictAllRecords();
	}
	else if(arg[1] == "--make-not")
	{
		ui.makeDictNotConverted();
	}
	else if(arg[1] == "--merge")
	{
		ui.mergeDictionaries();
	}
	else if(arg[1] == "--convert")
	{
		ui.convertFile();
	}
	else if(arg[1] == "--scripts")
	{
		ui.writeScriptLog();
	}
	else if(arg[1] == "--binary")
	{
		ui.writeBinaryLog();
	}
	else if(arg[1] == "--compare" && arg.size() == 4)
	{
		ui.writeDifferencesLog();
	}
	else
	{
		std::cout << "Syntax error!" << endl;
		std::cout << usage << endl;
	}
}
