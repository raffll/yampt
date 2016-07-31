#include "ui.hpp"

using namespace std;

//----------------------------------------------------------
Ui::Ui(vector<string> &a) : arg(a)
{

}

//----------------------------------------------------------
void Ui::makeDictRaw()
{
	Creator creator(arg[2]);
	creator.makeDict();
	creator.writeDict();
}

//----------------------------------------------------------
void Ui::makeDictBase()
{
	Creator creator(arg[2], arg[3]);
	creator.compareEsm();
	creator.makeDict();
	creator.writeDict();
}

//----------------------------------------------------------
void Ui::makeDictAllRecords()
{
	auto beg = arg.rbegin();
	auto end = arg.rend() - 3;
	vector<string> path(beg, end);
	Merger merger(path);
	merger.mergeDict();
	Creator creator(arg[2], merger);
	creator.makeDict();
	creator.writeDict();
}

//----------------------------------------------------------
void Ui::makeDictNotConverted()
{
	auto beg = arg.rbegin();
	auto end = arg.rend() - 3;
	vector<string> path(beg, end);
	Merger merger(path);
	merger.mergeDict();
	Creator creator(arg[2], merger, true);
	creator.makeDict();
	creator.writeDict();
}

//----------------------------------------------------------
void Ui::mergeDictionaries()
{
	auto beg = arg.rbegin();
	auto end = arg.rend() - 2;
	vector<string> path(beg, end);
	Merger merger(path);
	merger.mergeDict();
	merger.writeMerged();
	merger.writeLog();
}

//----------------------------------------------------------
void Ui::convertFile()
{
	auto beg = arg.rbegin();
	auto end = arg.rend() - 3;
	vector<string> path(beg, end);
	Merger merger(path);
	merger.mergeDict();
	merger.writeLog();
	Converter converter(arg[2], merger);
	converter.convertEsm();
	converter.writeEsm();
	if(converter.getConverterStatus() == 1)
	{
		Creator creator(arg[2], merger, true);
		creator.makeDict();
		creator.writeDict();
	}
}

//----------------------------------------------------------
void Ui::writeScriptLog()
{
	for(size_t i = 2; i < arg.size(); ++i)
	{
		Creator creator(arg[i]);
		creator.writeScripts();
	}
}

//----------------------------------------------------------
void Ui::writeBinaryLog()
{
	for(size_t i = 2; i < arg.size(); ++i)
	{
		Creator creator(arg[i]);
		creator.writeBinary();
	}
}

//----------------------------------------------------------
void Ui::writeDifferencesLog()
{
	auto beg = arg.rbegin();
	auto end = arg.rend() - 2;
	vector<string> path(beg, end);
	Merger merger(path);
	merger.writeDiff();
	merger.writeLog();
}
