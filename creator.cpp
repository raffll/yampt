#include "creator.hpp"

using namespace std;

//----------------------------------------------------------
Creator::Creator(string esm_path)
{
	esm.readEsm(esm_path);
	esm_ptr = &esm;
	if(esm.getEsmStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
Creator::Creator(string esm_path, string ext_path)
{
	esm.readEsm(esm_path);
	ext.readEsm(ext_path);
	esm_ptr = &ext;
	if(esm.getEsmStatus() == 1 && ext.getEsmStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
Creator::Creator(string esm_path, Merger &m, bool no_dupl)
{
	esm.readEsm(esm_path);
	esm_ptr = &esm;
	dict = m;
	if(esm.getEsmStatus() == 1 && dict.getMergerStatus() == 1)
	{
		status = 1;
		with_dict = 1;
		no_duplicates = no_dupl;
	}
}

//----------------------------------------------------------
void Creator::makeDict()
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
void Creator::writeDict()
{
	if(status == 1)
	{
		string name;
		if(no_duplicates == 1)
		{
			name = "yampt-notconverted-" + esm.getEsmPrefix() + ".log";
		}
		else
		{
			name = esm.getEsmPrefix() + ".dic";
		}
		if(!created.empty())
		{
			ofstream file(Config::output_path + name, ios::binary);
			for(const auto &elem : created)
			{
				file << "<!-------------------------------------------------------------->\r\n";
				file << Config::sep[1] << elem.first
				     << Config::sep[2] << elem.second
				     << Config::sep[3] << "\r\n";
			}
			Config::appendLog("--> Writing " + to_string(created.size()) +
					  " records to " + Config::output_path + name + "...\r\n");
		}
		else
		{
			Config::appendLog("--> No records to make dictionary!\r\n");
		}
	}
}

//----------------------------------------------------------
void Creator::writeScripts()
{
	if(status == 1)
	{
		string name = "yampt-scripts-" + esm.getEsmPrefix() + ".log";
		ofstream file(Config::output_path + name, ios::binary);
		esm.resetRec();
		while(esm.setNextRec())
		{
			if(esm.getRecId() == "SCPT")
			{
				esm.setRecContent();
				esm.setPriSubRec("SCHD");
				esm.setSecSubRec("SCTX");
				file << esm.getSecText()
				     << "\r\n----------------------------------------------------------\r\n";
			}
		}
		esm.resetRec();
		while(esm.setNextRec())
		{
			if(esm.getRecId() == "INFO")
			{
				esm.setRecContent();
				esm.setPriSubRec("INAM");
				esm.setSecSubRec("BNAM");
				file << esm.getSecText()
				     << "\r\n----------------------------------------------------------\r\n";
			}
		}
		Config::appendLog("--> Writing " + Config::output_path + name + "...\r\n");
	}
}

//----------------------------------------------------------
void Creator::compareEsm()
{
	if(status == 1)
	{
		string esm_compare;
		string ext_compare;
		esm.resetRec();
		ext.resetRec();
		while(esm.setNextRec())
		{
			esm_compare += esm.getRecId();
		}
		while(ext.setNextRec())
		{
			ext_compare += ext.getRecId();
		}
		if(esm_compare != ext_compare)
		{
			Config::appendLog("--> They are not the same master files!\r\n");
			status = 0;
		}
	}
}

//----------------------------------------------------------
void Creator::insertRecord(const string &pri, const string &sec)
{
	if(no_duplicates == 1)
	{
		auto search = dict.getDict().find(pri);
		if(search == dict.getDict().end())
		{
			if(created.insert({pri, sec}).second == 1)
			{
				counter++;
			}
		}
	}
	else
	{
		if(created.insert({pri, sec}).second == 1)
		{
			counter++;
		}
	}
}

//----------------------------------------------------------
string Creator::dialTranslator(string to_translate)
{
	if(with_dict == 1)
	{
		auto search = dict.getDict().find("DIAL" + Config::sep[0] + to_translate);
		if(search != dict.getDict().end())
		{
			return search->second;
		}
	}
	return to_translate;
}

//----------------------------------------------------------
string Creator::makeGap(string str)
{
	string ws(str.size(), ' ');
	return ws = "\n" + ws;
}

//----------------------------------------------------------
void Creator::makeDictCELL()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.setNextRec() && ext.setNextRec())
	{
		if(esm.getRecId() == "CELL")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			ext.setRecContent();
			ext.setPriSubRec("NAME");
			if(!esm.getPriText().empty())
			{
				insertRecord(esm.getRecId() + Config::sep[0] +
					     esm_ptr->getPriText(),
					     esm.getPriText());
			}
		}
	}
	Config::appendLog("    --> CELL records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictGMST()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.setNextRec() && ext.setNextRec())
	{
		if(esm.getRecId() == "GMST")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("STRV");
			ext.setRecContent();
			ext.setPriSubRec("NAME");
			ext.setSecSubRec("STRV");
			if(!esm.getSecText().empty())
			{
				insertRecord(esm.getRecId() + Config::sep[0] +
					     esm.getPriText(),
					     esm.getSecText());
			}
			if(esm.getPriText() == "sDefaultCellname")
			{
				insertRecord("CELL" + Config::sep[0] +
					     esm_ptr->getSecText(),
					     esm.getSecText());
			}
		}
	}
	Config::appendLog("    --> GMST records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictFNAM()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.setNextRec() && ext.setNextRec())
	{
		if(esm.getRecId() == "ACTI" ||
		   esm.getRecId() == "ALCH" ||
		   esm.getRecId() == "APPA" ||
		   esm.getRecId() == "ARMO" ||
		   esm.getRecId() == "BOOK" ||
		   esm.getRecId() == "BSGN" ||
		   esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "CLOT" ||
		   esm.getRecId() == "CONT" ||
		   esm.getRecId() == "CREA" ||
		   esm.getRecId() == "DOOR" ||
		   esm.getRecId() == "ENCH" ||
		   esm.getRecId() == "FACT" ||
		   esm.getRecId() == "INGR" ||
		   esm.getRecId() == "LIGH" ||
		   esm.getRecId() == "LOCK" ||
		   esm.getRecId() == "MISC" ||
		   esm.getRecId() == "NPC_" ||
		   esm.getRecId() == "PROB" ||
		   esm.getRecId() == "RACE" ||
		   esm.getRecId() == "REGN" ||
		   esm.getRecId() == "REPA" ||
		   esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "SPEL" ||
		   esm.getRecId() == "WEAP")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("FNAM");
			ext.setRecContent();
			ext.setPriSubRec("NAME");
			ext.setSecSubRec("FNAM");
			if(!esm.getPriText().empty())
			{
				insertRecord(esm.getSecId() + Config::sep[0] +
					     esm.getRecId() + Config::sep[0] +
					     esm.getPriText(),
					     esm.getSecText());
			}
			if(esm.getRecId() == "REGN")
			{
				insertRecord("CELL" + Config::sep[0] +
					     esm_ptr->getSecText(),
					     esm.getSecText());
			}
		}
	}
	Config::appendLog("    --> FNAM records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictDESC()
{
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		if(esm.getRecId() == "BSGN" ||
		   esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE")

		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DESC");
			insertRecord(esm.getSecId() + Config::sep[0] +
				     esm.getRecId() + Config::sep[0] +
				     esm.getPriText(),
				     esm.getSecText());
		}
	}
	Config::appendLog("    --> DESC records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictTEXT()
{
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		if(esm.getRecId() == "BOOK")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("TEXT");
			insertRecord(esm.getSecId() + Config::sep[0] +
				     esm.getPriText(),
				     esm.getSecText());
		}
	}
	Config::appendLog("    --> TEXT records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictRNAM()
{
	counter = 0;
	int rnam;
	esm.resetRec();
	ext.resetRec();
	while(esm.setNextRec() && ext.setNextRec())
	{
		if(esm.getRecId() == "FACT")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			ext.setRecContent();
			ext.setPriSubRec("NAME");
			rnam = 0;
			while(esm.setSecSubRec("RNAM", 4) && ext.setSecSubRec("RNAM", 4))
			{
				insertRecord(esm.getSecId() + Config::sep[0] +
					     esm.getPriText() + Config::sep[0] +
					     to_string(rnam),
					     esm.getSecText());
				rnam++;
			}
		}
	}
	Config::appendLog("    --> RNAM records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictINDX()
{
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setRecContent();
			esm.setPriSubRecINDX();
			esm.setSecSubRec("DESC");
			insertRecord(esm.getPriId() + Config::sep[0] +
				     esm.getRecId() + Config::sep[0] +
				     esm.getPriText(),
				     esm.getSecText());
		}
	}
	Config::appendLog("    --> INDX records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictDIAL()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.setNextRec() && ext.setNextRec())
	{
		if(esm.getRecId() == "DIAL")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			ext.setRecContent();
			ext.setPriSubRec("NAME");
			ext.setSecSubRec("DATA");
			if(esm.dialType() == "T")
			{
				insertRecord(esm.getRecId() + Config::sep[0] +
					     esm_ptr->getPriText(),
					     esm.getPriText());
			}
		}
	}
	Config::appendLog("    --> DIAL records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictINFO()
{
	counter = 0;
	string dial;
	esm.resetRec();
	while(esm.setNextRec())
	{
		if(esm.getRecId() == "DIAL")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			dial = esm.dialType() + Config::sep[0] + dialTranslator(esm.getPriText());
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setRecContent();
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("NAME");
			if(!esm.getSecText().empty())
			{
				insertRecord(esm.getRecId() + Config::sep[0] +
					     dial + Config::sep[0] +
					     esm.getPriText(),
					     esm.getSecText());
			}
		}
	}
	Config::appendLog("    --> INFO records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictBNAM()
{
	counter = 0;
	string dial;
	esm.resetRec();
	ext.resetRec();
	while(esm.setNextRec() && ext.setNextRec())
	{
		if(esm.getRecId() == "INFO")
		{
			esm.setRecContent();
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("BNAM");
			esm.setCollMessageOnly();
			ext.setRecContent();
			ext.setPriSubRec("INAM");
			ext.setSecSubRec("BNAM");
			ext.setCollMessageOnly();
			for(size_t i = 0; i < esm.getCollSize(); i++)
			{
				insertRecord(esm.getSecId() + Config::sep[0] +
					     esm_ptr->getCollLine(i),
					     makeGap(Config::sep[1] + esm.getSecId()) + Config::sep[0] +
					     esm.getCollLine(i));
			}
		}
	}
	Config::appendLog("    --> BNAM records created: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Creator::makeDictSCPT()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.setNextRec() && ext.setNextRec())
	{
		if(esm.getRecId() == "SCPT")
		{
			esm.setRecContent();
			esm.setPriSubRec("SCHD");
			esm.setSecSubRec("SCTX");
			esm.setCollMessageOnly();
			ext.setRecContent();
			ext.setPriSubRec("SCHD");
			ext.setSecSubRec("SCTX");
			ext.setCollMessageOnly();
			for(size_t i = 0; i < esm.getCollSize(); i++)
			{
				insertRecord(esm.getRecId() + Config::sep[0] +
					     esm_ptr->getCollLine(i),
					     makeGap(Config::sep[1] + esm.getRecId()) + Config::sep[0] +
					     esm.getCollLine(i));
			}
		}
	}
	Config::appendLog("    --> SCPT records created: " + to_string(counter) + "\r\n");
}
