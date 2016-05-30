#include "converter.hpp"

//----------------------------------------------------------
converter::converter(string esm_path, vector<string> dict_path) : dict_tool(dict_path)
{
	esm_tool.readEsm(esm_path);
	dict_tool.mergeDict();
}

//----------------------------------------------------------
void converter::printConverterLog(int i)
{
	cerr << dict_name[i] << " records converted: " << conv_counter << endl;
}

//----------------------------------------------------------
void converter::writeEsm()
{
	string esm_name = esm_tool.getEsmPrefix() + conv_suffix + esm_tool.getEsmSuffix();
	ofstream file(esm_name, ios::binary);
	file << esm_tool.getEsmContent();
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
	string conv_rec;
	string conv_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	conv_counter = 0;

	esm_tool.resetRec();
	while(esm_tool.loopCheck())
	{
		esm_tool.setNextRec();
		esm_tool.setRecContent();
		conv_rec = esm_tool.getRecContent();
		if(esm_tool.getRecId() == "CELL")
		{
			esm_tool.setPriSubRec("NAME");
			if(!esm_tool.getPriText().empty())
			{
				auto search = dict_tool.getDict(0).find(esm_tool.getPriText());
				if(search != dict_tool.getDict(0).end() && esm_tool.getPriText() != search->second)
				{
				    sec_size = search->second.size() + 1;
					conv_rec.erase(esm_tool.getPriPos() + 4, 4);
					conv_rec.insert(esm_tool.getPriPos() + 4, intToByte(sec_size));

					conv_rec.erase(esm_tool.getPriPos() + 8, esm_tool.getPriSize());
					conv_rec.insert(esm_tool.getPriPos() + 8, search->second + '\0');

                    rec_size = conv_rec.size() - 16;
					conv_rec.erase(4, 4);
					conv_rec.insert(4, intToByte(rec_size));

					conv_counter++;
				}
			}
		}
		conv_content.append(conv_rec);
	}
	esm_tool.setEsmContent(conv_content);
	printConverterLog(0);
}

//----------------------------------------------------------
void converter::convertGmst()
{
	string conv_rec;
	string conv_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	conv_counter = 0;

	esm_tool.resetRec();
	while(esm_tool.loopCheck())
	{
		esm_tool.setNextRec();
		esm_tool.setRecContent();
		conv_rec = esm_tool.getRecContent();
		if(esm_tool.getRecId() == "GMST")
		{
			esm_tool.setPriSubRec("NAME");
			esm_tool.setSecSubRec("STRV");
			if(!esm_tool.getSecText().empty())
			{
				auto search = dict_tool.getDict(1).find(esm_tool.getPriText());
				if(search != dict_tool.getDict(1).end() && esm_tool.getSecText() != search->second)
				{
				    sec_size = search->second.size() + 1;
					conv_rec.erase(esm_tool.getSecPos() + 4, 4);
					conv_rec.insert(esm_tool.getSecPos() + 4, intToByte(sec_size));

					conv_rec.erase(esm_tool.getSecPos() + 8, esm_tool.getSecSize());
					conv_rec.insert(esm_tool.getSecPos() + 8, search->second + '\0');

                    rec_size = conv_rec.size() - 16;
					conv_rec.erase(4, 4);
					conv_rec.insert(4, intToByte(rec_size));

					conv_counter++;
				}
			}
		}
		conv_content.append(conv_rec);
	}
	esm_tool.setEsmContent(conv_content);
	printConverterLog(1);
}

//----------------------------------------------------------
void converter::convertFnam()
{
	string conv_rec;
	string conv_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	conv_counter = 0;

	esm_tool.resetRec();
	while(esm_tool.loopCheck())
	{
		esm_tool.setNextRec();
		esm_tool.setRecContent();
		conv_rec = esm_tool.getRecContent();
		if(esm_tool.getRecId() == "ACTI" || esm_tool.getRecId() == "ALCH" ||
		   esm_tool.getRecId() == "APPA" || esm_tool.getRecId() == "ARMO" ||
		   esm_tool.getRecId() == "BOOK" || esm_tool.getRecId() == "CLAS" ||
		   esm_tool.getRecId() == "CLOT" || esm_tool.getRecId() == "CONT" ||
		   esm_tool.getRecId() == "CREA" || esm_tool.getRecId() == "DOOR" ||
		   esm_tool.getRecId() == "ENCH" || esm_tool.getRecId() == "FACT" ||
		   esm_tool.getRecId() == "INGR" || esm_tool.getRecId() == "LIGH" ||
		   esm_tool.getRecId() == "MISC" || esm_tool.getRecId() == "NPC_" ||
		   esm_tool.getRecId() == "PROB" || esm_tool.getRecId() == "REPA" ||
		   esm_tool.getRecId() == "SKIL" || esm_tool.getRecId() == "SPEL" ||
		   esm_tool.getRecId() == "WEAP")
		{
			esm_tool.setPriSubRec("NAME");
			esm_tool.setSecSubRec("FNAM");
			if(!esm_tool.getPriText().empty())
			{
				auto search = dict_tool.getDict(2).find(esm_tool.getRecId() + inner_sep + esm_tool.getPriText());
				if(search != dict_tool.getDict(2).end() && esm_tool.getSecText() != search->second)
				{
				    sec_size = search->second.size() + 1;
					conv_rec.erase(esm_tool.getSecPos() + 4, 4);
					conv_rec.insert(esm_tool.getSecPos() + 4, intToByte(sec_size));

					conv_rec.erase(esm_tool.getSecPos() + 8, esm_tool.getSecSize());
					conv_rec.insert(esm_tool.getSecPos() + 8, search->second + '\0');

                    rec_size = conv_rec.size() - 16;
					conv_rec.erase(4, 4);
					conv_rec.insert(4, intToByte(rec_size));

					conv_counter++;
				}
			}
		}
		conv_content.append(conv_rec);
	}
	esm_tool.setEsmContent(conv_content);
	printConverterLog(2);
}

//----------------------------------------------------------
void converter::convertDesc()
{
	string conv_rec;
	string conv_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	conv_counter = 0;

	esm_tool.resetRec();
	while(esm_tool.loopCheck())
	{
		esm_tool.setNextRec();
		esm_tool.setRecContent();
		conv_rec = esm_tool.getRecContent();
		if(esm_tool.getRecId() == "CLAS" ||
		   esm_tool.getRecId() == "RACE" ||
		   esm_tool.getRecId() == "BSGN")
		{
			esm_tool.setPriSubRec("NAME");
			esm_tool.setSecSubRec("DESC");
			if(!esm_tool.getPriText().empty())
			{
				auto search = dict_tool.getDict(3).find(esm_tool.getRecId() + inner_sep + esm_tool.getPriText());
				if(search != dict_tool.getDict(3).end() && esm_tool.getSecText() != search->second)
				{
				    sec_size = search->second.size() + 1;
					conv_rec.erase(esm_tool.getSecPos() + 4, 4);
					conv_rec.insert(esm_tool.getSecPos() + 4, intToByte(sec_size));

					conv_rec.erase(esm_tool.getSecPos() + 8, esm_tool.getSecSize());
					conv_rec.insert(esm_tool.getSecPos() + 8, search->second + '\0');

                    rec_size = conv_rec.size() - 16;
					conv_rec.erase(4, 4);
					conv_rec.insert(4, intToByte(rec_size));

					conv_counter++;
				}
			}
		}
		conv_content.append(conv_rec);
	}
	esm_tool.setEsmContent(conv_content);
	printConverterLog(3);
}
