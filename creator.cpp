#include "esmtools.hpp"
#include "creator.hpp"

using namespace std;

//----------------------------------------------------------
creator::creator(const char* path_base)
{
	base.readFile(path_base);
	esm_ptr = &base;
}

//----------------------------------------------------------
creator::creator(const char* path_base, const char* path_extd)
{
	base.readFile(path_base);
	extd.readFile(path_extd);
	esm_ptr = &extd;
}

//----------------------------------------------------------
void creator::makeDict()
{
	if(base.getStatus() == 1 && esm_ptr->getStatus() == 1)
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
		makeDictScpt();
	}
}

//----------------------------------------------------------
void creator::printLog(int i)
{
	cerr << dict_name[i] << " records created: " << counter << endl;
}

//----------------------------------------------------------
void creator::writeDict()
{
	for(int i = 0; i < 10; i++)
	{
		ofstream file;
		if(!dict[i].empty())
		{
			file.open(dict_name[i].c_str());
			for(const auto &elem : dict[i])
			{
				file << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
			}
			cerr << "--> Writing " << dict_name[i] << endl;
		}
		else
		{
			cerr << "--> Skipping " << dict_name[i] << endl;
		}
	}
}

//----------------------------------------------------------
void creator::makeDictCell()
{
	counter = 0;
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "CELL")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			extd.setRecContent();
			extd.setPriSubRec("NAME");
			if(!base.getPriText().empty())
			{
				dict[0].insert({esm_ptr->getPriText(), base.getPriText()});
				counter++;
			}
		}
	}
	printLog(0);
}

//----------------------------------------------------------
void creator::makeDictGmst()
{
	counter = 0;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		if(base.getRecId() == "GMST")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("STRV");
			if(!base.getSecText().empty())
			{
				dict[1].insert({base.getPriText(), base.getSecText()});
				counter++;
			}
		}
	}
	printLog(1);
}

//----------------------------------------------------------
void creator::makeDictFnam()
{
	counter = 0;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		if(base.getRecId() == "ACTI" || base.getRecId() == "ALCH" ||
		   base.getRecId() == "APPA" || base.getRecId() == "ARMO" ||
		   base.getRecId() == "BOOK" || base.getRecId() == "CLAS" ||
		   base.getRecId() == "CLOT" || base.getRecId() == "CONT" ||
		   base.getRecId() == "CREA" || base.getRecId() == "DOOR" ||
		   base.getRecId() == "ENCH" || base.getRecId() == "FACT" ||
		   base.getRecId() == "INGR" || base.getRecId() == "LIGH" ||
		   base.getRecId() == "MISC" || base.getRecId() == "NPC_" ||
		   base.getRecId() == "PROB" || base.getRecId() == "REPA" ||
		   base.getRecId() == "SKIL" || base.getRecId() == "SPEL" ||
		   base.getRecId() == "WEAP")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("FNAM");
			if(!base.getPriText().empty())
			{
				dict[2].insert({base.getRecId() + inner_sep + base.getPriText(), base.getSecText()});
				counter++;
			}
		}
	}
	printLog(2);
}

//----------------------------------------------------------
void creator::makeDictDesc()
{
	counter = 0;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		if(base.getRecId() == "CLAS" ||
		   base.getRecId() == "RACE" ||
		   base.getRecId() == "BSGN")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("DESC");
			dict[3].insert({base.getRecId() + inner_sep + base.getPriText(), base.getSecText()});
			counter++;
		}
	}
	printLog(3);
}

//----------------------------------------------------------
void creator::makeDictBook()
{
	counter = 0;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		if(base.getRecId() == "BOOK")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("TEXT");
			dict[4].insert({base.getPriText(), base.getSecText()});
			counter++;
		}
	}
	printLog(4);
}

//----------------------------------------------------------
void creator::makeDictFact()
{
	counter = 0;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		if(base.getRecId() == "FACT")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("RNAM");
			for(unsigned i = 0; i < base.getTmpSize(); i++)
			{
				dict[5].insert({base.getPriText() + inner_sep + to_string(i), base.getTmpLine(i)});
				counter++;
			}
		}
	}
	printLog(5);
}

//----------------------------------------------------------
void creator::makeDictIndx()
{
	counter = 0;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		if(base.getRecId() == "SKIL" ||
		   base.getRecId() == "MGEF")
		{
			base.setRecContent();
			base.setPriSubRec("INDX");
			base.setSecSubRec("DESC");
			dict[6].insert({base.getRecId() + inner_sep + base.getPriText(), base.getSecText()});
			counter++;
		}
	}
	printLog(6);
}

//----------------------------------------------------------
void creator::makeDictDial()
{
	counter = 0;
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "DIAL")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("DATA");
			extd.setRecContent();
			extd.setPriSubRec("NAME");
			extd.setSecSubRec("DATA");
			if(base.dialType() == "T")
			{
				dict[7].insert({esm_ptr->getPriText(), base.getPriText()});
				counter++;
			}
		}
	}
	printLog(7);
}

//----------------------------------------------------------
void creator::makeDictInfo()
{
	counter = 0;
	string dial;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		if(base.getRecId() == "DIAL")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("DATA");
			dial = base.dialType() + inner_sep + base.getPriText();
		}
		if(base.getRecId() == "INFO")
		{
			base.setRecContent();
			base.setPriSubRec("INAM");
			base.setSecSubRec("NAME");
			if(!base.getSecText().empty())
			{
				dict[8].insert({dial + inner_sep + base.getPriText(), base.getSecText()});
				counter++;
			}
		}
	}
	printLog(8);
}

//----------------------------------------------------------
void creator::makeDictScpt()
{
	counter = 0;
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "INFO")
		{
			base.setRecContent();
			base.setPriSubRec("INAM");
			base.setSecSubRec("BNAM");
			extd.setRecContent();
			extd.setPriSubRec("INAM");
			extd.setSecSubRec("BNAM");
			for(unsigned i = 0; i < base.getTmpSize(); i++)
			{
				for(unsigned j = 0; j < key.size(); j++)
				{
					if(base.getTmpLine(i).find(key[j]) != string::npos)
					{
						dict[9].insert({base.getPriText() + inner_sep + esm_ptr->getTmpLine(i), base.getTmpLine(i)});
						counter++;
					}
				}
			}
		}
	}
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "SCPT")
		{
			base.setRecContent();
			base.setPriSubRec("SCHD");
			base.setSecSubRec("SCTX");
			extd.setRecContent();
			extd.setPriSubRec("SCHD");
			extd.setSecSubRec("SCTX");
			for(unsigned i = 0; i < base.getTmpSize(); i++)
			{
				for(unsigned j = 0; j < key.size(); j++)
				{
					if(base.getTmpLine(i).find(key[j]) != string::npos)
					{
						dict[9].insert({base.getPriText() + inner_sep + esm_ptr->getTmpLine(i), base.getTmpLine(i)});
						counter++;
					}
				}
			}
		}
	}
	printLog(9);
}
