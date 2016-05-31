#include "creator.hpp"

using namespace std;

//----------------------------------------------------------
creator::creator(string esm_path)
{
	esm.readEsm(esm_path);
	esm_ptr = &esm;
}

//----------------------------------------------------------
creator::creator(string esm_path, string ext_path)
{
	esm.readEsm(esm_path);
	ext.readEsm(ext_path);
	esm_ptr = &ext;
}

//----------------------------------------------------------
void creator::makeDict()
{
	if(esm.getEsmStatus() == 1 && esm_ptr->getEsmStatus() == 1)
	{
		makeDictCell();
		makeDictGmst();
		makeDictFnam();
		makeDictDesc();
		makeDictBook();
		makeDictFact();
		makeDictIndx();
		makeDictDial();
		makeDictInfo();
		makeDictBnam();
		makeDictScpt();
		cerr << "Creating complete!" << endl;
	}
}

//----------------------------------------------------------
void creator::writeDict()
{
	if(esm.getEsmStatus() == 1 && esm_ptr->getEsmStatus() == 1)
	{
		ofstream file;
		file.open(esm.getEsmPrefix() + ".dic");
		for(const auto &elem : dict)
		{
			file << sep[1] << elem.first << sep[2] << elem.second << sep[3] << endl;
		}
		cerr << "Writing " << esm.getEsmPrefix() << ".dic..." << endl;
	}
}

//----------------------------------------------------------
void creator::makeDictCell()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		ext.setNextRec();
		if(esm.getRecId() == "CELL")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			ext.setRecContent();
			ext.setPriSubRec("NAME");
			if(!esm.getPriText().empty())
			{
				dict.insert({esm.getRecId() + sep[0] + esm_ptr->getPriText(), esm.getPriText()});
				counter++;
			}
		}
	}
	cerr << "CELL records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictGmst()
{
	counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		if(esm.getRecId() == "GMST")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("STRV");
			if(!esm.getSecText().empty())
			{
				dict.insert({esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
				counter++;
			}
		}
	}
	cerr << "GMST records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictFnam()
{
	counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		if(esm.getRecId() == "ACTI" || esm.getRecId() == "ALCH" ||
		   esm.getRecId() == "APPA" || esm.getRecId() == "ARMO" ||
		   esm.getRecId() == "BOOK" || esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "CLOT" || esm.getRecId() == "CONT" ||
		   esm.getRecId() == "CREA" || esm.getRecId() == "DOOR" ||
		   esm.getRecId() == "ENCH" || esm.getRecId() == "FACT" ||
		   esm.getRecId() == "INGR" || esm.getRecId() == "LIGH" ||
		   esm.getRecId() == "MISC" || esm.getRecId() == "NPC_" ||
		   esm.getRecId() == "PROB" || esm.getRecId() == "REPA" ||
		   esm.getRecId() == "SKIL" || esm.getRecId() == "SPEL" ||
		   esm.getRecId() == "WEAP")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("FNAM");
			if(!esm.getPriText().empty())
			{
				dict.insert({esm.getSecId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
				counter++;
			}
		}
	}
	cerr << "FNAM records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictDesc()
{
	counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		if(esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE" ||
		   esm.getRecId() == "BSGN")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DESC");
			dict.insert({esm.getSecId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
			counter++;
		}
	}
	cerr << "DESC records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictBook()
{
	counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		if(esm.getRecId() == "BOOK")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("TEXT");
			dict.insert({esm.getSecId() + sep[0] + esm.getPriText(), esm.getSecText()});
			counter++;
		}
	}
	cerr << "TEXT records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictFact()
{
	counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		if(esm.getRecId() == "FACT")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("RNAM");
			for(size_t i = 0; i < esm.getTmpSize(); i++)
			{
				dict.insert({esm.getSecId() + sep[0] + esm.getPriText() + sep[0] + to_string(i), esm.getTmpLine(i)});
				counter++;
			}
		}
	}
	cerr << "RNAM records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictIndx()
{
	counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setRecContent();
			esm.setPriSubRec("INDX");
			esm.setSecSubRec("DESC");
			dict.insert({esm.getPriId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
			counter++;
		}
	}
	cerr << "INDX records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictDial()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		ext.setNextRec();
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
				dict.insert({esm.getRecId() + sep[0] + esm_ptr->getPriText(), esm.getPriText()});
				counter++;
			}
		}
	}
	cerr << "DIAL records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictInfo()
{
	counter = 0;
	string dial;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		if(esm.getRecId() == "DIAL")
		{
			esm.setRecContent();
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			dial = esm.dialType() + sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setRecContent();
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("NAME");
			if(!esm.getSecText().empty())
			{
				dict.insert({esm.getRecId() + sep[0] + dial + sep[0] + esm.getPriText(), esm.getSecText()});
				counter++;
			}
		}
	}
	cerr << "INFO records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictBnam()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		ext.setNextRec();
		if(esm.getRecId() == "INFO")
		{
			esm.setRecContent();
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("BNAM");
			ext.setRecContent();
			ext.setPriSubRec("INAM");
			ext.setSecSubRec("BNAM");
			for(size_t i = 0; i < esm.getTmpSize(); i++)
			{
				for(size_t j = 0; j < key.size(); j++)
				{
					if(esm.getTmpLine(i).find(key[j]) != string::npos)
					{
						dict.insert({esm.getSecId() + sep[0] + esm.getPriText() + sep[0] + esm_ptr->getTmpLine(i), esm.getTmpLine(i)});
						counter++;
					}
				}
			}
		}
	}
	cerr << "BNAM records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::makeDictScpt()
{
	counter = 0;
	esm.resetRec();
	ext.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		ext.setNextRec();
		if(esm.getRecId() == "SCPT")
		{
			esm.setRecContent();
			esm.setPriSubRec("SCHD");
			esm.setSecSubRec("SCTX");
			ext.setRecContent();
			ext.setPriSubRec("SCHD");
			ext.setSecSubRec("SCTX");
			for(unsigned i = 0; i < esm.getTmpSize(); i++)
			{
				for(unsigned j = 0; j < key.size(); j++)
				{
					if(esm.getTmpLine(i).find(key[j]) != string::npos)
					{
						dict.insert({esm.getRecId() + sep[0] + esm.getPriText() + sep[0] + esm_ptr->getTmpLine(i), esm.getTmpLine(i)});
						counter++;
					}
				}
			}
		}
	}
	cerr << "SCPT records created: " << counter << endl;
}
