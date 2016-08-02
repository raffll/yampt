#include "ui.hpp"

int main(int argc, char *argv[])
{
	std::vector<std::string> arg;
	for(int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	Config config;
	config.readConfig();
	Ui ui(arg);

	std::string usage =
	"Usage: " + arg[0] + " [command]"
	"\n"
	"\n  --help                                Print this message."
	"\n  --make-all  -f [file]... -d [dict]... Make dictionary from esp/esm plugin with all records."
	"\n  --make-not  -f [file]... -d [dict]... Make without records from dictionary."
	"\n  --make-raw  -f [file]...              Like \"--make-all\", but without DIAL translation."
	"\n  --make-base -f [file] [file]          Make base dictionary from two different localized esm files."
	"\n  --compare   -d [dict] [dict]          Compare two dictionaries and create differences log."
	"\n  --merge     -d [dict]...              Validate,  merge, sort and delete doubled records."
	"\n  --convert   -f [file]... -d [dict]... Convert plugin from dictionaries in paths"
	"\n                                        and create dictionary as \"--make-not\" command."
	"\n  --scripts   -f [file]...              Write scripts content log.";

	if(arg[1] == "--help")
	{
		std::cout << usage << endl;
	}
	else if(arg[1] == "--make-raw")
	{
		ui.makeDictRaw();
	}
	else if(arg[1] == "--make-base")
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
