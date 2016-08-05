#include "ui.hpp"

int main(int argc, char *argv[])
{
	std::vector<std::string> arg;
	std::vector<std::string> arg_file;
	std::vector<std::string> arg_dict;
	for(int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	enum command { null, file, dict };
	command comm = null;
	for(size_t i = 0; i < arg.size(); ++i)
	{
		if(arg[i] == "-f")
		{
			comm = file;
		}
		else if(arg[i] == "-d")
		{
			comm = dict;
		}
		else
		{
			if(comm == file)
			{
				arg_file.push_back(arg[i]);
			}
			if(comm == dict)
			{
				arg_dict.push_back(arg[i]);
			}
		}
	}
	vector<string> arg_dict_rev(arg_dict.rbegin(), arg_dict.rend());

	std::string usage = "Usage: " + arg[0] + " [command]"
			    "\n"
			    "\n  --help                                "
			    "Print this message."
			    "\n  --make-all  -f [file]... -d [dict]... "
			    "Make dictionary from esp/esm plugin with all records."
			    "\n  --make-not  -f [file]... -d [dict]... "
			    "Make without records from dictionary."
			    "\n  --make-raw  -f [file]...              "
			    "Like \"--make-all\", but without DIAL translation."
			    "\n  --make-base -f [file_n] [file_f]      "
			    "Make base dictionary from two different localized esm files."
			    "\n  --compare   -d [dict] [dict]          "
			    "Compare two dictionaries and create differences log."
			    "\n  --merge     -d [dict]...              "
			    "Validate, merge, sort and delete doubled records."
			    "\n  --convert   -f [file]... -d [dict]... "
			    "Convert plugin from dictionaries in paths"
			    "\n                                        "
			    "and create dictionary as \"--make-not\" command."
			    "\n  --scripts   -f [file]...              "
			    "Write scripts content log.";

	if(arg.size() > 1)
	{
		Ui ui;

		if(arg[1] == "--help")
		{
			std::cout << usage << endl;
		}
		else if(arg[1] == "--make-raw" && arg_file.size() > 1)
		{
			ui.makeDictRaw(arg_file);
		}
		else if(arg[1] == "--make-base" && arg_file.size() == 2)
		{
			ui.makeDictBase(arg_file);
		}
		else if(arg[1] == "--make-all" && arg_file.size() > 1 && arg_dict_rev.size() > 1)
		{
			ui.makeDictAllRecords(arg_file, arg_dict_rev);
		}
		else if(arg[1] == "--make-not" && arg_file.size() > 1 && arg_dict_rev.size() > 1)
		{
			ui.makeDictNotConverted(arg_file, arg_dict_rev);
		}
		else if(arg[1] == "--merge" && arg_dict_rev.size() > 1)
		{
			ui.mergeDictionaries(arg_dict_rev);
		}
		else if(arg[1] == "--convert" && arg_file.size() > 1 && arg_dict_rev.size() > 1)
		{
			ui.convertFile(arg_file, arg_dict_rev);
		}
		else if(arg[1] == "--scripts" && arg_file.size() > 1)
		{
			ui.writeScriptLog(arg_file);
		}
		else if(arg[1] == "--compare" && arg_dict.size() == 2)
		{
			ui.writeDifferencesLog(arg_dict);
		}
		else
		{
			std::cout << "Syntax error!" << endl;
			std::cout << usage << endl;
		}
	}
	else
	{
		std::cout << "Syntax error!" << endl;
		std::cout << usage << endl;
	}
}
