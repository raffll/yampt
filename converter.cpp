#include "converter.hpp"

//----------------------------------------------------------
converter::converter(string esm_path, merger &m)
{
	esm.readEsm(esm_path);
	dict = m;
	if(esm.getEsmStatus() == 1 && dict.getMergerStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
void converter::convertEsm()
{
	if(status == 1)
	{
		convertCell();
		convertGmst();
		convertFnam();
		convertDesc();
		convertBook();
		convertFact();
		convertIndx();
		convertDial();
		convertInfo();
		convertBnam();
		cerr << "Converting complete!" << endl;
	}
}

//----------------------------------------------------------
void converter::writeEsm()
{
	if(status == 1)
	{
		string name = esm.getEsmPrefix() + ".converted" + esm.getEsmSuffix();
		ofstream file(name, ios::binary);
		file << esm.getEsmContent();
		cout << "Writing " << name << "..." << endl;
	}
}

//----------------------------------------------------------
string converter::intToByte(unsigned int x)
{
	char bytes[4];
	string str;
	copy(static_cast<const char*>(static_cast<const void*>(&x)),
	     static_cast<const char*>(static_cast<const void*>(&x)) + sizeof x,
	     bytes);
	for(int i = 0; i < 4; i++)
	{
		str.push_back(bytes[i]);
	}
	return str;
}

//----------------------------------------------------------
void converter::convertCell()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "CELL")
		{
			esm.setPriSubRec("NAME");
			auto search = dict.getDict().find(esm.getRecId() + sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getPriPos() + 8, esm.getPriSize());
					rec_content.insert(esm.getPriPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getPriPos() + 4, 4);
					rec_content.insert(esm.getPriPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "CELL records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertGmst()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "GMST")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("STRV");
			auto search = dict.getDict().find(esm.getRecId() + sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "GMST records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertFnam()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
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
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("FNAM");
			auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getRecId() +
							  sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "FNAM records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertDesc()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE" ||
		   esm.getRecId() == "BSGN")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DESC");
			auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getRecId() +
							  sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "DESC records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertBook()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "BOOK")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("TEXT");
			auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "TEXT records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertFact()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string rnam_text;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "FACT")
		{
			esm.setPriSubRec("NAME");
			esm.setRnamColl();
			for(size_t i = 0; i < esm.getCollSize(); i++)
			{
				auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getPriText() +
								  sep[0] + to_string(i) + sep[0] + esm.getRnamText(i));
				if(search != dict.getDict().end())
				{
					if(esm.getSecText() != search->second)
					{
						// subrecord text
						rnam_text = search->second;
						rnam_text.resize(32);
						rec_content.erase(esm.getRnamPos(i) + 8, esm.getSecSize());
						rec_content.insert(esm.getRnamPos(i) + 8, rnam_text);
						// subrecord size
						sec_size = 32;
						rec_content.erase(esm.getRnamPos(i) + 4, 4);
						rec_content.insert(esm.getRnamPos(i) + 4, intToByte(sec_size));
						// record size
						rec_size = rec_content.size() - 16;
						rec_content.erase(4, 4);
						rec_content.insert(4, intToByte(rec_size));
					}
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "RNAM records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertIndx()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setPriSubRec("INDX");
			esm.setSecSubRec("DESC");
			auto search = dict.getDict().find(esm.getPriId() + sep[0] + esm.getRecId() +
							  sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "INDX records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertDial()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "DIAL")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			auto search = dict.getDict().find(esm.getRecId() + sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getPriPos() + 8, esm.getPriSize());
					rec_content.insert(esm.getPriPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getPriPos() + 4, 4);
					rec_content.insert(esm.getPriPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "DIAL records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertInfo()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string dial;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "DIAL")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			dial = esm.dialType() + sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("NAME");
			auto search = dict.getDict().find(esm.getRecId() + sep[0] + dial +
							  sep[0] + esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					// subrecord text
					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');
					// subrecord size
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));
					// record size
					rec_size = rec_content.size() - 16;
					rec_content.erase(4, 4);
					rec_content.insert(4, intToByte(rec_size));
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "INFO records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertBnam()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string sec_text;
	size_t pos_text = 0;
	string dial;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "DIAL")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			dial = esm.dialType() + sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("BNAM");
			esm.setScptColl(true);
			for(size_t i = 0; i < esm.getCollSize(); i++)
			{
				auto search = dict.getDict().find(esm.getRecId() + sep[0] + dial + sep[0] + esm.getPriText() + sep[0] + esm.getScptText(i));
				if(search != dict.getDict().end())
				{
						// subrecord text
						sec_text = esm.getSecText();
						pos_text = sec_text.find(esm.getScptText(i));
						sec_text.erase(pos_text, esm.getScptText(i).size());
						sec_text.insert(pos_text, search->second);
						rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
						rec_content.insert(esm.getSecPos() + 8, sec_text);
						// subrecord size
						sec_size = sec_text.size();
						rec_content.erase(esm.getSecPos() + 4, 4);
						rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));
						// record size
						rec_size = rec_content.size() - 16;
						rec_content.erase(4, 4);
						rec_content.insert(4, intToByte(rec_size));
						counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "BNAM records converted: " << counter << endl;
}
