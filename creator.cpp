#include "esmtools.hpp"
#include "creator.hpp"

using namespace std;

//----------------------------------------------------------
creator::creator(const char* b) : base(b), extd()
{
	esm_ptr = &base;

	if(base.getStatus() == 1)
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
creator::creator(const char* b, const char* e) : base(b), extd(e)
{
	esm_ptr = &extd;

	if(base.getStatus() == 1 && extd.getStatus() == 1)
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
void creator::writeDictAll()
{
	for(int i = 0; i < 10; i++)
	{
		writeDict(i);
	}
}

//----------------------------------------------------------
void creator::writeDict(int i)
{
	ofstream file;
	string file_name = "dict_" + to_string(i) + "_" + dict_name[i] + ".txt";
	file.open(file_name.c_str());
	for(const auto &elem : dict[i])
	{
		file << "<h3>" << elem.first << "</h3>" << elem.second << "<hr>" << endl;
	}
	file.close();
}

//----------------------------------------------------------
void creator::printDict(int i)
{
	for(const auto &elem : dict[i])
	{
		cout << "<h3>" << elem.first << "</h3>" << elem.second << "<hr>" << endl;
	}
}

//----------------------------------------------------------
//
//----------------------------------------------------------
void creator::makeDictCell()
{
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
			}
		}
	}
}

//----------------------------------------------------------
void creator::makeDictGmst()
{
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "GMST")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("STRV");
			extd.setRecContent();
			extd.setPriSubRec("NAME");
			extd.setSecSubRec("STRV");
			if(!base.getSecText().empty())
			{
				dict[1].insert({base.getPriText(), base.getSecText()});
			}
		}
	}
}

//----------------------------------------------------------
void creator::makeDictFnam()
{
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
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
			extd.setRecContent();
			extd.setPriSubRec("NAME");
			extd.setSecSubRec("FNAM");
			if(!base.getPriText().empty())
			{
				dict[2].insert({base.getRecId() + inner_sep + base.getPriText(), base.getSecText()});
			}
		}
	}
}

//----------------------------------------------------------
void creator::makeDictDesc()
{
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "CLAS" ||
		   base.getRecId() == "RACE" ||
		   base.getRecId() == "BSGN")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("DESC");
			extd.setRecContent();
			extd.setPriSubRec("NAME");
			extd.setSecSubRec("DESC");
			dict[3].insert({base.getRecId() + inner_sep + base.getPriText(), base.getSecText()});
		}
	}
}

//----------------------------------------------------------
void creator::makeDictBook()
{
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "BOOK")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("TEXT");
			extd.setRecContent();
			extd.setPriSubRec("NAME");
			extd.setSecSubRec("TEXT");
			dict[4].insert({base.getPriText(), base.getSecText()});
		}
	}
}

//----------------------------------------------------------
void creator::makeDictFact()
{
	base.resetRec();
	//extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		//extd.setNextRec();
		if(base.getRecId() == "FACT")
		{
			base.setRecContent();
			base.setPriSubRec("NAME");
			base.setSecSubRec("RNAM");
			//extd.setRecContent();
			//extd.setPriSubRec("NAME");
			//extd.setSecSubRec("RNAM");
			for(unsigned i = 0; i < base.temp_text.size(); i++)
			{
				dict[5].insert({base.getPriText() + inner_sep + to_string(i), base.temp_text[i]});
			}
		}
	}
}

//----------------------------------------------------------
void creator::makeDictIndx()
{
	base.resetRec();
	extd.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		extd.setNextRec();
		if(base.getRecId() == "SKIL" ||
		   base.getRecId() == "MGEF")
		{
			base.setRecContent();
			base.setPriSubRec("INDX");
			base.setSecSubRec("DESC");
			extd.setRecContent();
			extd.setPriSubRec("INDX");
			extd.setSecSubRec("DESC");
			dict[6].insert({base.getRecId() + inner_sep + base.getPriText(), base.getSecText()});
		}
	}
}

//----------------------------------------------------------
void creator::makeDictDial()
{
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
			}
		}
	}
}

//----------------------------------------------------------
void creator::makeDictInfo()
{
	string dial;
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
			//extd.setRecContent();
			//extd.setPriSubRec("NAME");
			//extd.setSecSubRec("DATA");
			dial = base.dialType() + inner_sep + base.getPriText();
		}
		if(base.getRecId() == "INFO")
		{
			base.setRecContent();
			base.setPriSubRec("INAM");
			base.setSecSubRec("NAME");
			extd.setRecContent();
			//extd.setPriSubRec("INAM");
			extd.setSecSubRec("NAME");
			dict[8].insert({dial + inner_sep + base.getPriText(), base.getSecText()});
		}
	}
}

//----------------------------------------------------------
void creator::makeDictScpt()
{
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
			for(unsigned i = 0; i < base.temp_text.size(); i++)
			{
				for(unsigned j = 0; j < key.size(); j++)
				{
					if(base.temp_text[i].find(key[j]) != string::npos)
					{
						dict[9].insert({base.getPriText() + inner_sep + esm_ptr->temp_text[i], base.temp_text[i]});
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
			for(unsigned i = 0; i < base.temp_text.size(); i++)
			{
				for(unsigned j = 0; j < key.size(); j++)
				{
					if(base.temp_text[i].find(key[j]) != string::npos)
					{
						dict[9].insert({base.getPriText() + inner_sep + esm_ptr->temp_text[i], base.temp_text[i]});
					}
				}
			}
		}
	}
}
