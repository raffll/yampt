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
creator::creator(string esm_path, merger &m)
{
	esm.readEsm(esm_path);
	esm_ptr = &esm;
	dict = m;
	with_dict = 1;
}

//----------------------------------------------------------
void creator::makeDict()
{
	if(not_equal == 0 && esm.getEsmStatus() == 1 && esm_ptr->getEsmStatus() == 1)
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
void creator::compareEsm()
{
	if(esm.getEsmStatus() == 1 && ext.getEsmStatus() == 1)
	{
		string esm_compare;
		string ext_compare;
		esm.resetRec();
		ext.resetRec();
		while(esm.loopCheck())
		{
			esm.setNextRec();
			esm_compare += esm.getRecId();
		}
		while(ext.loopCheck())
		{
			ext.setNextRec();
			ext_compare += ext.getRecId();
		}
		if(esm_compare != ext_compare)
		{
			cerr << "They are not the same master files!" << endl;
			not_equal = 1;
		}
	}
}

//----------------------------------------------------------
void creator::eraseDuplicates()
{
	if(with_dict == 1 && esm.getEsmStatus() == 1 && dict.getMergerStatus() == 1)
	{
		int duplicate = 0;
		for(auto &elem : dict.getDict())
		{
			auto search = created.find(elem.first);
			if(search != created.end() && elem.second == search->second)
			{
				created.erase(search);
				duplicate++;
			}
		}
		cerr << "Erase duplicate records found in dictionary: " << duplicate << endl;
	}
}

//----------------------------------------------------------
void creator::eraseDifferent()
{
	if(with_dict == 1 && esm.getEsmStatus() == 1 && dict.getMergerStatus() == 1)
	{
		int different = 0;
		for(auto &elem : dict.getDict())
		{
			auto search = created.find(elem.first);
			if(search != created.end())
			{
				created.erase(search);
				different++;
			}
		}
		cerr << "Erase duplicate records with different text: " << different << endl;
	}
}

//----------------------------------------------------------
string creator::dialTranslator(string to_translate)
{
	if(with_dict == 1 && esm.getEsmStatus() == 1 && dict.getMergerStatus() == 1)
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
void creator::writeDict()
{
	if(not_equal == 0 && esm.getEsmStatus() == 1 && esm_ptr->getEsmStatus() == 1)
	{
		if(!created.empty())
		{
			ofstream file;
			file.open(esm.getEsmPrefix() + ".dic");
			for(const auto &elem : created)
			{
				file << sep[1] << elem.first << sep[2] << elem.second << sep[3] << endl;
			}
			cerr << "Writing " << esm.getEsmPrefix() << ".dic..." << endl;
		}
		else
		{
			cerr << "All records converted!" << endl;
		}
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
				created.insert({esm.getRecId() + sep[0] + esm_ptr->getPriText(), esm.getPriText()});
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
				created.insert({esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
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
				created.insert({esm.getSecId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
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
			created.insert({esm.getSecId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
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
			created.insert({esm.getSecId() + sep[0] + esm.getPriText(), esm.getSecText()});
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
				created.insert({esm.getSecId() + sep[0] + esm.getPriText() + sep[0] + to_string(i), esm.getTmpLine(i)});
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
			created.insert({esm.getPriId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText(), esm.getSecText()});
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
				created.insert({esm.getRecId() + sep[0] + esm_ptr->getPriText(), esm.getPriText()});
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
			dial = esm.dialType() + sep[0] + dialTranslator(esm.getPriText());
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setRecContent();
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("NAME");
			if(!esm.getSecText().empty())
			{
				created.insert({esm.getRecId() + sep[0] + dial + sep[0] + esm.getPriText(), esm.getSecText()});
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
						created.insert({esm.getSecId() + sep[0] + esm.getPriText() + sep[0] + esm_ptr->getTmpLine(i),
								esm.getTmpLine(i)});
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
						created.insert({esm.getRecId() + sep[0] + esm.getPriText() + sep[0] + esm_ptr->getTmpLine(i),
								esm.getTmpLine(i)});
						counter++;
					}
				}
			}
		}
	}
	cerr << "SCPT records created: " << counter << endl;
}
