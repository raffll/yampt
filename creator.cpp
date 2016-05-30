#include "creator.hpp"

using namespace std;

//----------------------------------------------------------
creator::creator(string path_base)
{
	base.readEsm(path_base);
	esm_ptr = &base;
}

//----------------------------------------------------------
creator::creator(string path_base, string path_extd)
{
	base.readEsm(path_base);
	extd.readEsm(path_extd);
	esm_ptr = &extd;
}

//----------------------------------------------------------
void creator::makeDict()
{
	if(base.getEsmStatus() == 1 && esm_ptr->getEsmStatus() == 1)
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
		cerr << "Creating complete!" << endl;
	}
}

//----------------------------------------------------------
void creator::writeDict()
{
	for(int i = 0; i < 10; i++)
	{
		ofstream file;
		if(!dict[i].empty())
		{
			file.open(dict_name[i]);
			for(const auto &elem : dict[i])
			{
				file << line_sep[0] << elem.first << line_sep[1] << elem.second << line_sep[2] << endl;
			}
			cerr << "Writing " << dict_name[i] << endl;
		}
		else
		{
			cerr << "Skipping " << dict_name[i] << endl;
		}
	}
}

//----------------------------------------------------------
void creator::makeDictCell()
{
	rec_counter = 0;
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
				rec_counter++;
			}
		}
	}
	cerr << dict_name[0] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictGmst()
{
	rec_counter = 0;
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
				rec_counter++;
			}
		}
	}
	cerr << dict_name[1] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictFnam()
{
	rec_counter = 0;
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
				rec_counter++;
			}
		}
	}
	cerr << dict_name[2] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictDesc()
{
	rec_counter = 0;
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
			rec_counter++;
		}
	}
	cerr << dict_name[3] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictBook()
{
	rec_counter = 0;
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
			rec_counter++;
		}
	}
	cerr << dict_name[4] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictFact()
{
	rec_counter = 0;
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
				rec_counter++;
			}
		}
	}
	cerr << dict_name[5] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictIndx()
{
	rec_counter = 0;
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
			rec_counter++;
		}
	}
	cerr << dict_name[6] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictDial()
{
	rec_counter = 0;
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
				rec_counter++;
			}
		}
	}
	cerr << dict_name[7] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictInfo()
{
	rec_counter = 0;
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
				rec_counter++;
			}
		}
	}
	cerr << dict_name[8] << " records created: " << rec_counter << endl;
}

//----------------------------------------------------------
void creator::makeDictScpt()
{
	rec_counter = 0;
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
						rec_counter++;
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
						rec_counter++;
					}
				}
			}
		}
	}
	cerr << dict_name[9] << " records created: " << rec_counter << endl;
}
