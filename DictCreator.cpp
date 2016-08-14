#include "DictCreator.hpp"

using namespace std;

//----------------------------------------------------------
DictCreator::DictCreator(string esm_path)
{
	esm_n.readFile(esm_path);
	esm_ptr = &esm_n;
	if(esm_n.getStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(string esm_path, string ext_path)
{
	esm_n.readFile(esm_path);
	esm_f.readFile(ext_path);
	esm_ptr = &esm_f;
	if(esm_n.getStatus() == 1 && esm_f.getStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(string esm_path, DictMerger &m, bool no_dupl)
{
	esm_n.readFile(esm_path);
	esm_ptr = &esm_n;
	dict_merged = m;
	if(esm_n.getStatus() == 1 && dict_merged.getStatus() == 1)
	{
		status = 1;
		with_dict = 1;
		no_duplicates = no_dupl;
	}
}

//----------------------------------------------------------
void DictCreator::makeDict()
{
	if(status == 1)
	{
		makeDictCELL();
		makeDictGMST();
		makeDictFNAM();
		makeDictDESC();
		makeDictTEXT();
		makeDictRNAM();
		makeDictINDX();
		makeDictDIAL();
		makeDictINFO();
		makeDictBNAM();
		makeDictSCPT();
		Config::appendLog("--> Creating complete!\r\n");
	}
}

//----------------------------------------------------------
void DictCreator::writeDict()
{
	if(status == 1)
	{
		string name;
		if(no_duplicates == 1)
		{
			name = "yampt-notconverted-" + esm_n.getNamePrefix() + ".log";
		}
		else
		{
			name = esm_n.getNamePrefix() + ".dic";
		}
		if(!dict_created.empty())
		{
			ofstream file(Config::output_path + name, ios::binary);
			for(const auto &elem : dict_created)
			{
				file << Config::sep_line
				     << Config::sep[1] << elem.first
				     << Config::sep[2] << elem.second
				     << Config::sep[3] << "\r\n";
			}
			Config::appendLog("--> Writing " + to_string(dict_created.size()) +
					  " records to " + Config::output_path + name + "...\r\n");
		}
		else
		{
			Config::appendLog("--> No records to make dictionary!\r\n");
		}
	}
}

//----------------------------------------------------------
void DictCreator::writeScripts()
{
	if(status == 1)
	{
		string name = "yampt-scripts-" + esm_n.getNamePrefix() + ".log";
		ofstream file(Config::output_path + name, ios::binary);
		for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
		{
			esm_n.setRec(i);
			if(esm_n.getRecId() == "SCPT")
			{
				esm_n.setPri("SCHD");
				esm_n.setSec("SCTX");
				file << esm_n.getSecText() << "\r\n" << Config::sep_line;
			}
		}
		for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
		{
			esm_n.setRec(i);
			if(esm_n.getRecId() == "INFO")
			{
				esm_n.setPri("INAM");
				esm_n.setSec("BNAM");
				file << esm_n.getSecText() << "\r\n" << Config::sep_line;
			}
		}
		Config::appendLog("--> Writing " + Config::output_path + name + "...\r\n");
	}
}

//----------------------------------------------------------
void DictCreator::compareEsm()
{
	if(status == 1)
	{
		if(esm_n.getRecColl().size() == esm_f.getRecColl().size())
		{
			string esm_compare;
			string ext_compare;
			for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
			{
				esm_compare += esm_n.getRecId();
			}
			for(size_t i = 0; i < esm_f.getRecColl().size(); ++i)
			{
				ext_compare += esm_f.getRecId();
			}
			if(esm_compare != ext_compare)
			{
				Config::appendLog("--> They are not the same master files!\r\n");
				status = 0;
			}
		}
		else
		{
			Config::appendLog("--> They are not the same master files!\r\n");
			status = 0;
		}
	}
}

//----------------------------------------------------------
void DictCreator::insertRecord(const string &pri_text, const string &sec_text, int dict_num)
{
	if(no_duplicates == 1)
	{
		auto search = dict_merged.getDict(dict_num).find(pri_text);
		if(search == dict_merged.getDict(dict_num).end())
		{
			if(dict_created.insert({pri_text, sec_text}).second == 1)
			{
				counter++;
			}
		}
	}
	else
	{
		if(dict_created.insert({pri_text, sec_text}).second == 1)
		{
			counter++;
		}
	}
}

//----------------------------------------------------------
string DictCreator::dialTranslator(string to_translate)
{
	if(with_dict == 1)
	{
		auto search = dict_merged.getDict(7).find("DIAL" + Config::sep[0] + to_translate);
		if(search != dict_merged.getDict(7).end())
		{
			return search->second;
		}
	}
	return to_translate;
}

//----------------------------------------------------------
string DictCreator::makeGap(string str)
{
	string ws(str.size(), ' ');
	return ws = "\n" + ws;
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "CELL")
		{
			esm_n.setPri("NAME");
			esm_f.setPri("NAME");
			if(!esm_n.getPriText().empty())
			{
				insertRecord(esm_n.getRecId() + Config::sep[0] +
					     esm_ptr->getPriText(),
					     esm_n.getPriText(), 0);
			}
		}
	}
	Config::appendLog("    --> CELL records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setPri("NAME");
			esm_n.setSec("STRV");
			esm_f.setPri("NAME");
			esm_f.setSec("STRV");
			if(!esm_n.getSecText().empty())
			{
				insertRecord(esm_n.getRecId() + Config::sep[0] +
					     esm_n.getPriText(),
					     esm_n.getSecText(), 1);
			}
			if(esm_n.getPriText() == "sDefaultCellname")
			{
				insertRecord("CELL" + Config::sep[0] +
					     esm_ptr->getSecText(),
					     esm_n.getSecText(), 0);
			}
		}
	}
	Config::appendLog("    --> GMST records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "ACTI" ||
		   esm_n.getRecId() == "ALCH" ||
		   esm_n.getRecId() == "APPA" ||
		   esm_n.getRecId() == "ARMO" ||
		   esm_n.getRecId() == "BOOK" ||
		   esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "CLOT" ||
		   esm_n.getRecId() == "CONT" ||
		   esm_n.getRecId() == "CREA" ||
		   esm_n.getRecId() == "DOOR" ||
		   esm_n.getRecId() == "ENCH" ||
		   esm_n.getRecId() == "FACT" ||
		   esm_n.getRecId() == "INGR" ||
		   esm_n.getRecId() == "LIGH" ||
		   esm_n.getRecId() == "LOCK" ||
		   esm_n.getRecId() == "MISC" ||
		   esm_n.getRecId() == "NPC_" ||
		   esm_n.getRecId() == "PROB" ||
		   esm_n.getRecId() == "RACE" ||
		   esm_n.getRecId() == "REGN" ||
		   esm_n.getRecId() == "REPA" ||
		   esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "SPEL" ||
		   esm_n.getRecId() == "WEAP")
		{
			esm_n.setPri("NAME");
			esm_n.setSec("FNAM");
			esm_f.setPri("NAME");
			esm_f.setSec("FNAM");
			if(!esm_n.getPriText().empty())
			{
				insertRecord(esm_n.getSecId() + Config::sep[0] +
					     esm_n.getRecId() + Config::sep[0] +
					     esm_n.getPriText(),
					     esm_n.getSecText(), 2);
			}
			if(esm_n.getRecId() == "REGN")
			{
				insertRecord("CELL" + Config::sep[0] +
					     esm_ptr->getSecText(),
					     esm_n.getSecText(), 0);
			}
		}
	}
	Config::appendLog("    --> FNAM records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "RACE")

		{
			esm_n.setPri("NAME");
			esm_n.setSec("DESC");
			insertRecord(esm_n.getSecId() + Config::sep[0] +
				     esm_n.getRecId() + Config::sep[0] +
				     esm_n.getPriText(),
				     esm_n.getSecText(), 3);
		}
	}
	Config::appendLog("    --> DESC records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "BOOK")
		{
			esm_n.setPri("NAME");
			esm_n.setSec("TEXT");
			insertRecord(esm_n.getSecId() + Config::sep[0] +
				     esm_n.getPriText(),
				     esm_n.getSecText(), 4);
		}
	}
	Config::appendLog("    --> TEXT records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "FACT")
		{
			esm_n.setPri("NAME");
			esm_n.setSecColl("RNAM");
			for(size_t j = 0; j < esm_n.getSecCollSize(); j++)
			{
				insertRecord(esm_n.getSecId() + Config::sep[0] +
					     esm_n.getPriText() + Config::sep[0] +
					     to_string(j),
					     esm_n.getSecText(j), 5);
			}
		}
	}
	Config::appendLog("    --> RNAM records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "MGEF")
		{
			esm_n.setPriINDX();
			esm_n.setSec("DESC");
			insertRecord(esm_n.getPriId() + Config::sep[0] +
				     esm_n.getRecId() + Config::sep[0] +
				     esm_n.getPriText(),
				     esm_n.getSecText(), 6);
		}
	}
	Config::appendLog("    --> INDX records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setPri("NAME");
			esm_n.setSec("DATA");
			esm_f.setPri("NAME");
			esm_f.setSec("DATA");
			if(esm_n.getDialType() == "T")
			{
				insertRecord(esm_n.getRecId() + Config::sep[0] +
					     esm_ptr->getPriText(),
					     esm_n.getPriText(), 7);
			}
		}
	}
	Config::appendLog("    --> DIAL records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
	counter = 0;
	string dial;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setPri("NAME");
			esm_n.setSec("DATA");
			dial = esm_n.getDialType() + Config::sep[0] + dialTranslator(esm_n.getPriText());
		}
		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setPri("INAM");
			esm_n.setSec("NAME");
			if(!esm_n.getSecText().empty())
			{
				insertRecord(esm_n.getRecId() + Config::sep[0] +
					     dial + Config::sep[0] +
					     esm_n.getPriText(),
					     esm_n.getSecText(), 8);
			}
		}
	}
	Config::appendLog("    --> INFO records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
	counter = 0;
	string dial;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setPri("INAM");
			esm_n.setSec("BNAM");
			esm_n.setMessageCollOnly(esm_n.getSecText());
			esm_f.setPri("INAM");
			esm_f.setSec("BNAM");
			esm_f.setMessageCollOnly(esm_f.getSecText());
			for(size_t i = 0; i < esm_n.getScptCollSize(); i++)
			{
				insertRecord(esm_n.getSecId() + Config::sep[0] +
					     esm_ptr->getScptLine(i),
					     makeGap(Config::sep[1] + esm_n.getSecId()) + Config::sep[0] +
					     esm_n.getScptLine(i), 9);
			}
		}
	}
	Config::appendLog("    --> BNAM records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "SCPT")
		{
			esm_n.setPri("SCHD");
			esm_n.setSec("SCTX");
			esm_n.setMessageCollOnly(esm_n.getSecText());
			esm_f.setPri("SCHD");
			esm_f.setSec("SCTX");
			esm_f.setMessageCollOnly(esm_f.getSecText());
			for(size_t i = 0; i < esm_n.getScptCollSize(); i++)
			{
				insertRecord(esm_n.getRecId() + Config::sep[0] +
					     esm_ptr->getScptLine(i),
					     makeGap(Config::sep[1] + esm_n.getRecId()) + Config::sep[0] +
					     esm_n.getScptLine(i), 10);
			}
		}
	}
	Config::appendLog("    --> SCPT records created: " + to_string(counter) + "\r\n");
}
