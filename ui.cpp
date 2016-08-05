#include "ui.hpp"

using namespace std;

//----------------------------------------------------------
void Ui::makeDictRaw(vector<string> &arg_file)
{
	Config config;
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		Creator creator(arg_file[i]);
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void Ui::makeDictBase(vector<string> &arg_file)
{
	Config config;
	if(arg_file.size() == 2)
	{
		Creator creator(arg_file[0], arg_file[1]);
		creator.compareEsm();
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void Ui::makeDictAllRecords(vector<string> &arg_file, vector<string> &arg_dict_rev)
{
	Config config;
	Merger merger(arg_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		Creator creator(arg_file[i], merger);
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void Ui::makeDictNotConverted(vector<string> &arg_file, vector<string> &arg_dict_rev)
{
	Config config;
	Merger merger(arg_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		Creator creator(arg_file[i], merger, true);
		creator.makeDict();
		creator.writeDict();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void Ui::mergeDictionaries(vector<string> &arg_dict_rev)
{
	Config config;
	Merger merger(arg_dict_rev);
	merger.mergeDict();
	merger.writeMerged();
	Config::writeLog();
}

//----------------------------------------------------------
void Ui::convertFile(vector<string> &arg_file, vector<string> &arg_dict_rev)
{
	Config config;
	Merger merger(arg_dict_rev);
	merger.mergeDict();
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		Converter converter(arg_file[i], merger);
		converter.convertEsm();
		converter.writeEsm();
		if(converter.getConverterStatus() == 1)
		{
			Creator creator(arg_file[i], merger, true);
			creator.makeDict();
			creator.writeDict();
		}
	}
	Config::writeLog();
}

//----------------------------------------------------------
void Ui::writeScriptLog(vector<string> &arg_file)
{
	Config config;
	for(size_t i = 0; i < arg_file.size(); ++i)
	{
		Creator creator(arg_file[i]);
		creator.writeScripts();
	}
	Config::writeLog();
}

//----------------------------------------------------------
void Ui::writeDifferencesLog(vector<string> &arg_dict)
{
	Config config;
	if(arg_dict.size() == 2)
	{
		Merger merger(arg_dict);
		merger.writeDiff();
	}
	Config::writeLog();
}
