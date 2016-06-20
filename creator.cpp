#include "creator.hpp"

using namespace std;

//----------------------------------------------------------
creator::creator(string esm_path)
{
	esm.readEsm(esm_path);
	esm_ptr = &esm;
	if(esm.getEsmStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
creator::creator(string esm_path, string ext_path)
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
creator::creator(string esm_path, merger &m, bool no_dupl)
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
void creator::makeDict()
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
		cerr << "Creating complete!" << endl;
	}
}

//----------------------------------------------------------
void creator::writeDict()
{
	if(status == 1)
	{
		string suffix;
		if(no_duplicates == 1)
		{
			suffix = "-NotFoundInDict.dic";
		}
		else
		{
			suffix = ".dic";
		}
		if(!created.empty())
		{
			ofstream file;
			file.open(esm.getEsmPrefix() + suffix, ios::binary);
			for(const auto &elem : created)
			{
				file << sep[1] << elem.first << sep[2] << elem.second << sep[3] << "\r\n";
			}
			cerr << "Writing " << created.size()
			     << " records to " << esm.getEsmPrefix()
			     << suffix << "..." << endl;
		}
		else
		{
			cerr << "No records to make dictionary!" << endl;
		}
	}
}

//----------------------------------------------------------
void creator::writeScripts()
{
	if(status == 1)
	{
		ofstream file;
		file.open("Scripts-" + esm.getEsmPrefix() + ".log", ios::binary);
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
		cerr << "Writing Scripts-" << esm.getEsmPrefix() << ".log..." << endl;
	}
}

//----------------------------------------------------------
void creator::compareEsm()
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
			cerr << "They are not the same master files!" << endl;
			status = 0;
		}
	}
}

//----------------------------------------------------------
void creator::insertRecord(const string &pri, const string &sec)
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
string creator::dialTranslator(string to_translate)
{
	if(with_dict == 1)
	{
		auto search = dict.getDict().find("DIAL" + sep[0] + to_translate);
		if(search != dict.getDict().end())
		{
			return search->second;
		}
	}
	return to_translate;
}

//----------------------------------------------------------
string creator::makeGap(string str)
{
	string ws(str.size(), ' ');
	return ws = "\n" + ws;
}

//----------------------------------------------------------
void creator::makeDictCELL()
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
				insertRecord(esm.getRecId() + sep[0] +
					     esm_ptr->getPriText(),
					     esm.getPriText());
			}
		}
	}
	cerr << "CELL records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictGMST()
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
				insertRecord(esm.getRecId() + sep[0] +
					     esm.getPriText(),
					     esm.getSecText());
			}
			if(esm.getPriText() == "sDefaultCellname")
			{
				insertRecord("CELL" + sep[0] +
					     esm_ptr->getSecText(),
					     esm.getSecText());
			}
		}
	}
	cerr << "GMST records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictFNAM()
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
				insertRecord(esm.getSecId() + sep[0] +
					     esm.getRecId() + sep[0] +
					     esm.getPriText(),
					     esm.getSecText());
			}
			if(esm.getRecId() == "REGN")
			{
				insertRecord("CELL" + sep[0] +
					     esm_ptr->getSecText(),
					     esm.getSecText());
			}
		}
	}
	cerr << "FNAM records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictDESC()
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
			insertRecord(esm.getSecId() + sep[0] +
				     esm.getRecId() + sep[0] +
				     esm.getPriText(),
				     esm.getSecText());
		}
	}
	cerr << "DESC records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictTEXT()
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
			insertRecord(esm.getSecId() + sep[0] +
				     esm.getPriText(),
				     esm.getSecText());
		}
	}
	cerr << "TEXT records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictRNAM()
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
				insertRecord(esm.getSecId() + sep[0] +
					     esm.getPriText() + sep[0] +
					     to_string(rnam) + sep[0] +
					     esm_ptr->getSecText(),
					     esm.getSecText());
				rnam++;
			}
		}
	}
	cerr << "RNAM records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictINDX()
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
			insertRecord(esm.getPriId() + sep[0] +
				     esm.getRecId() + sep[0] +
				     esm.getPriText(),
				     esm.getSecText());
		}
	}
	cerr << "INDX records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictDIAL()
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
				insertRecord(esm.getRecId() + sep[0] +
					     esm_ptr->getPriText(),
					     esm.getPriText());
			}
		}
	}
	cerr << "DIAL records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictINFO()
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
			dial = esm.dialType() + sep[0] + dialTranslator(esm.getPriText());
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setRecContent();
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("NAME");
			if(!esm.getSecText().empty())
			{
				insertRecord(esm.getRecId() + sep[0] +
					     dial + sep[0] +
					     esm.getPriText(),
					     esm.getSecText());
			}
		}
	}
	cerr << "INFO records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictBNAM()
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
				insertRecord(esm.getSecId() + sep[0] +
					     esm_ptr->getCollText(i),
					     makeGap(sep[1] + esm.getSecId()) + sep[0] +
					     esm.getCollText(i));
			}
		}
	}
	cerr << "BNAM records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictSCPT()
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
				insertRecord(esm.getRecId() + sep[0] +
					     esm_ptr->getCollText(i),
					     makeGap(sep[1] + esm.getRecId()) + sep[0] +
					     esm.getCollText(i));
			}
		}
	}
	cerr << "SCPT records created: " << counter << endl;
}
