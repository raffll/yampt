#include "converter.hpp"

//----------------------------------------------------------
converter::converter(string esm_path, merger &m)
{
	esm.readEsm(esm_path);
	dict = m;
}

//----------------------------------------------------------
void converter::convertEsm()
{
	convertCell();
	convertGmst();
	convertFnam();
	convertDesc();
	cerr << "Converting complete!" << endl;
}

//----------------------------------------------------------
void converter::writeEsm()
{
	string name = esm.getEsmPrefix() + ".converted" + esm.getEsmSuffix();
	ofstream file(name, ios::binary);
	file << esm.getEsmContent();
	cout << "Writing " << name << "..." << endl;
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
	counter = 0;

	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "CELL")
		{
			esm.setPriSubRec("NAME");
			if(!esm.getPriText().empty())
			{
				auto search = dict.getDict().find(esm.getRecId() + sep[0] + esm.getPriText());
				if(search != dict.getDict().end() && esm.getPriText() != search->second)
				{
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getPriPos() + 4, 4);
					rec_content.insert(esm.getPriPos() + 4, intToByte(sec_size));

					rec_content.erase(esm.getPriPos() + 8, esm.getPriSize());
					rec_content.insert(esm.getPriPos() + 8, search->second + '\0');

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
	cerr << "CELL records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertGmst()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	counter = 0;

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
			if(!esm.getSecText().empty())
			{
				auto search = dict.getDict().find(esm.getRecId() + sep[0] + esm.getPriText());
				if(search != dict.getDict().end() && esm.getSecText() != search->second)
				{
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));

					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');

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
	cerr << "GMST records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertFnam()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	counter = 0;

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
			if(!esm.getPriText().empty())
			{
				auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText());
				if(search != dict.getDict().end() && esm.getSecText() != search->second)
				{
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));

					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');

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
	cerr << "FNAM records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertDesc()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	counter = 0;

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
			if(!esm.getPriText().empty())
			{
				auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getRecId() + sep[0] + esm.getPriText());
				if(search != dict.getDict().end() && esm.getSecText() != search->second)
				{
					sec_size = search->second.size() + 1;
					rec_content.erase(esm.getSecPos() + 4, 4);
					rec_content.insert(esm.getSecPos() + 4, intToByte(sec_size));

					rec_content.erase(esm.getSecPos() + 8, esm.getSecSize());
					rec_content.insert(esm.getSecPos() + 8, search->second + '\0');

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
	cerr << "DESC records converted: " << counter << endl;
}

