#include "UserInterface.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	vector<string> arg;
	for(int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	string usage = "Usage: " + arg[0] + " [command]"
			"\n"
			"\n  --help                                      Print this message."
			"\n  --make-all  -f <file_list> -d <dict_list>   Make dictionary from esp/esm plugin with all records."
			"\n  --make-not  -f <file_list> -d <dict_list>   Make without records from dictionary."
			"\n  --make-raw  -f <file_list>                  Like \"--make-all\", but without DIAL translation."
			"\n  --make-base -f [file_native] [file_foreign] Make base dictionary from two different"
			"\n                                              localized esm files."
			"\n  --compare   -d [dict_first] [dict_second]   Compare two dictionaries and create differences log."
			"\n  --merge     -d <dict_list>                  Validate, merge, sort and delete doubled records."
			"\n  --convert   -f <file_list> -d <dict_list>   Convert plugin from dictionaries in paths"
			"\n                                              and create dictionary as \"--make-not\" command."
			"\n  --scripts   -f <file_list>                  Write scripts content log.";

	if(arg.size() > 1)
	{
		Config config();
		UserInterface ui(arg);

		if(arg[1] == "--help")
		{
			cout << usage << endl;
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
			ui.makeDictAll();
		}
		else if(arg[1] == "--make-not")
		{
			ui.makeDictNot();
		}
		else if(arg[1] == "--merge")
		{
			ui.mergeDict();
		}
		else if(arg[1] == "--convert")
		{
			ui.convertEsm();
		}
		else if(arg[1] == "--scripts")
		{
			ui.writeScripts();
		}
		else if(arg[1] == "--compare")
		{
			ui.writeCompare();
		}
		else
		{
			cout << "Syntax error!" << endl;
		}
	}
	else
	{
		cout << "Syntax error!" << endl;
	}
}
