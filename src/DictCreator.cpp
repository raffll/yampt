#include "DictCreator.hpp"

using namespace std;

//----------------------------------------------------------
DictCreator::DictCreator(string path_n)
{
	esm_n.readFile(path_n);
	esm_ptr = &esm_n;
	if(esm_n.getStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(string path_n, string path_f)
{
	esm_n.readFile(path_n);
	esm_f.readFile(path_f);
	esm_ptr = &esm_f;
	if(esm_n.getStatus() == 1 && esm_f.getStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(string path_n, DictMerger &m, bool no_dupl)
{
	esm_n.readFile(path_n);
	esm_ptr = &esm_n;
	merger = m;
	if(esm_n.getStatus() == 1 && merger.getStatus() == 1)
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
	}
}

//----------------------------------------------------------
void DictCreator::writeScripts()
{
	if(status == 1)
	{
		string name = esm_n.getNamePrefix() + ".scripts.log";
		ofstream file(name, ios::binary);
		for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
		{
			esm_n.setRec(i);
			if(esm_n.getRecId() == "SCPT")
			{
				esm_n.setPri("SCHD");
				esm_n.setSec("SCTX");
				file << esm_n.getSecText() << "\r\n" << sep[4];
			}
		}
		for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
		{
			esm_n.setRec(i);
			if(esm_n.getRecId() == "INFO")
			{
				esm_n.setPri("INAM");
				esm_n.setSec("BNAM");
				file << esm_n.getSecText() << "\r\n" << sep[4];
			}
		}
		cout << "--> Writing " << name << "...\r\n";
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
				cout << "--> They are not the same master files!\r\n";
				status = 0;
			}
		}
		else
		{
			cout << "--> They are not the same master files!\r\n";
			status = 0;
		}
	}
}

//----------------------------------------------------------
void DictCreator::insertRecord(const string &pri_text, const string &sec_text, RecType i, bool extra)
{
	if(no_duplicates == 1)
	{
		auto search = merger.getDict()[i].find(pri_text);
		if(search == merger.getDict()[i].end())
		{
			dict[i].insert({pri_text, sec_text});
			if(extra == 1)
			{
				counter_cell++;
			}
			else
			{
				counter++;
			}
		}
	}
	else
	{
		dict[i].insert({pri_text, sec_text});
		if(extra == 1)
		{
			counter_cell++;
		}
		else
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
		auto search = merger.getDict()[RecType::DIAL].find("DIAL" + sep[0] + to_translate);
		if(search != merger.getDict()[RecType::DIAL].end())
		{
			return search->second;
		}
	}
	return to_translate;
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
				insertRecord("CELL" + sep[0] + esm_ptr->getPriText(),
					     esm_n.getPriText(),
					     RecType::CELL);
			}
		}
	}
	cout << "    --> CELL records created: " << to_string(counter) + "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
	counter = 0;
	counter_cell = 0;
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
				insertRecord("GMST" + sep[0] + esm_n.getPriText(),
					     esm_n.getSecText(),
					     RecType::GMST);
			}
			if(esm_n.getPriText() == "sDefaultCellname")
			{
				insertRecord("CELL" + sep[0] + esm_ptr->getSecText(),
					     esm_n.getSecText(),
					     RecType::CELL,
					     true);
			}
		}
	}
	cout << "    --> GMST records created: " << to_string(counter) + "\r\n";
	if(counter_cell > 0)
	{
		cout << "        + extra " + to_string(counter_cell) + " CELL records\r\n";
	}
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
	counter = 0;
	counter_cell = 0;
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
				insertRecord("FNAM" + sep[0] + esm_n.getRecId() + sep[0] + esm_n.getPriText(),
					     esm_n.getSecText(),
					     RecType::FNAM);
			}
			if(esm_n.getRecId() == "REGN")
			{
				insertRecord("CELL" + sep[0] + esm_ptr->getSecText(),
					     esm_n.getSecText(),
					     RecType::CELL,
					     true);
			}
		}
	}
	cout << "    --> FNAM records created: " << to_string(counter) << "\r\n";
	if(counter_cell > 0)
	{
		cout << "        + extra " + to_string(counter_cell) + " CELL records\r\n";
	}
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		if(esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "RACE")
		{
			esm_n.setPri("NAME");
			esm_n.setSec("DESC");
			insertRecord("DESC" + sep[0] + esm_n.getRecId() + sep[0] + esm_n.getPriText(),
				     esm_n.getSecText(),
				     RecType::DESC);
		}
	}
	cout << "    --> DESC records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		if(esm_n.getRecId() == "BOOK")
		{
			esm_n.setPri("NAME");
			esm_n.setSec("TEXT");
			insertRecord("TEXT" + sep[0] + esm_n.getPriText(),
				     esm_n.getSecText(),
				     RecType::TEXT);
		}
	}
	cout << "    --> TEXT records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		if(esm_n.getRecId() == "FACT")
		{
			esm_n.setPri("NAME");
			esm_n.setSecColl("RNAM");
			for(size_t k = 0; k < esm_n.getSecColl().size(); ++k)
			{
				insertRecord("RNAM" + sep[0] + esm_n.getPriText() + sep[0] + to_string(k),
					     esm_n.getSecText(k),
					     RecType::RNAM);
			}
		}
	}
	cout << "    --> RNAM records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		if(esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "MGEF")
		{
			esm_n.setPriINDX();
			esm_n.setSec("DESC");
			insertRecord("INDX" + sep[0] + esm_n.getRecId() + sep[0] + esm_n.getPriText(),
				     esm_n.getSecText(),
				     RecType::INDX);
		}
	}
	cout << "    --> INDX records created: " << to_string(counter) << "\r\n";
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
			esm_n.setSecDialType("DATA");
			esm_f.setPri("NAME");
			if(esm_n.getDialType() == "T")
			{
				insertRecord("DIAL" + sep[0] + esm_ptr->getPriText(),
					     esm_n.getPriText(),
					     RecType::DIAL);
			}
		}
	}
	cout << "    --> DIAL records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
	counter = 0;
	string dial;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setPri("NAME");
			esm_n.setSecDialType("DATA");
			dial = esm_n.getDialType() + sep[0] + dialTranslator(esm_n.getPriText());
		}
		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setPri("INAM");
			esm_n.setSec("NAME");
			if(!esm_n.getSecText().empty())
			{
				insertRecord("INFO" + sep[0] + dial + sep[0] + esm_n.getPriText(),
					     esm_n.getSecText(),
					     RecType::INFO);
			}
		}
	}
	cout << "    --> INFO records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
	counter = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setSecMessageColl("BNAM");
			esm_f.setSecMessageColl("BNAM");
			for(size_t k = 0; k < esm_n.getScptColl().size(); ++k)
			{
				insertRecord("BNAM" + sep[0] + esm_ptr->getScptLine(k),
					     sep[5] + sep[0] + esm_n.getScptLine(k),
					     RecType::BNAM);
			}
		}
	}
	cout << "    --> BNAM records created: " << to_string(counter) << "\r\n";
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
			esm_n.setSecMessageColl("SCTX");
			esm_f.setSecMessageColl("SCTX");
			for(size_t k = 0; k < esm_n.getScptColl().size(); ++k)
			{
				insertRecord("SCTX" + sep[0] + esm_ptr->getScptLine(k),
					     sep[5] + sep[0] + esm_n.getScptLine(k),
					     RecType::SCTX);
			}
		}
	}
	cout << "    --> SCTX records created: " << to_string(counter) << "\r\n";
}
