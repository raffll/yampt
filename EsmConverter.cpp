#include "EsmConverter.hpp"

using namespace std;

//----------------------------------------------------------
EsmConverter::EsmConverter(string esm_path, DictMerger &m)
{
	esm.readFile(esm_path);
	dict = m;
	if(esm.getStatus() == 1 && dict.getStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
void EsmConverter::convertEsm()
{
	if(status == 1)
	{
		convertCELL();
		convertPGRD();
		convertANAM();
		convertSCVR();
		convertDNAM();
		convertCNDT();
		convertGMST();
		convertFNAM();
		convertDESC();
		convertTEXT();
		convertRNAM();
		convertINDX();
		convertDIAL();
		convertINFO();
		convertBNAM();
		convertSCPT();
		Config::appendLog("--> Converting complete!\r\n");
	}
}

//----------------------------------------------------------
void EsmConverter::writeEsm()
{
	if(status == 1)
	{
		string name = esm.getNamePrefix() + Config::output_suffix + esm.getNameSuffix();
		ofstream file(Config::output_path + name, ios::binary);
		for(auto &elem : esm.getRecColl())
		{
			file << elem;
		}
		Config::appendLog("--> Writing " + Config::output_path + name + "...\r\n");
	}
}

//----------------------------------------------------------
string EsmConverter::intToByte(unsigned int x)
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
bool EsmConverter::caseInsensitiveStringCmp(string lhs, string rhs)
{
	transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);
	transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);
	if(lhs == rhs)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent(size_t pos, size_t old_size, string new_text,
					size_t new_size, size_t rec_num)
{
	unsigned int rec_size;
	rec_content = esm.getRecContent(rec_num);
	rec_content.erase(pos + 8, old_size);
	rec_content.insert(pos + 8, new_text);
	rec_content.erase(pos + 4, 4);
	rec_content.insert(pos + 4, intToByte(new_size));
	rec_size = rec_content.size() - 16;
	rec_content.erase(4, 4);
	rec_content.insert(4, intToByte(rec_size));
	esm.setRecContent(rec_num, rec_content);
}

//----------------------------------------------------------
void EsmConverter::convertScriptLine(int i)
{
	bool found = 0;
	string line = esm.getScptLine(i);
	string text = esm.getScptText(i);
	if(esm.getScptLineType(i) == "BNAM")
	{
		auto search = dict.getDict(9).find("BNAM" + Config::sep[0] + esm.getScptLine(i));
		if(search != dict.getDict(9).end())
		{
			line = search->second;
			line = line.substr(line.find(Config::sep[0]) + Config::sep[0].size());
			found = 1;
			if(esm.getScptLine(i) != line)
			{
				counter++;
			}
		}
	}
	else if(esm.getScptLineType(i) == "SCPT")
	{
		auto search = dict.getDict(10).find("SCPT" + Config::sep[0] + esm.getScptLine(i));
		if(search != dict.getDict(10).end())
		{
			line = search->second;
			line = line.substr(line.find(Config::sep[0]) + Config::sep[0].size());
			found = 1;
			if(esm.getScptLine(i) != line)
			{
				counter++;
			}
		}
	}
	else if(esm.getScptLineType(i) == "DIAL")
	{
		auto search = dict.getDict(7).find("DIAL" + Config::sep[0] + text);
		if(search != dict.getDict(7).end())
		{

			line.erase(esm.getScptTextPos(i), esm.getScptText(i).size() + 2);
			line.insert(esm.getScptTextPos(i), "\"" + search->second + "\"");
			counter++;
			found = 1;
		}
		if(found == 0)
		{
			for(auto &elem : dict.getDict(7))
			{
				if(caseInsensitiveStringCmp("DIAL" + Config::sep[0] + text, elem.first) == 1)
				{
					line.erase(esm.getScptTextPos(i), esm.getScptText(i).size() + 2);
					line.insert(esm.getScptTextPos(i), "\"" + elem.second + "\"");
					counter++;
					found = 1;
					break;
				}
			}
		}
	}
	else if(esm.getScptLineType(i) == "CELL")
	{
		auto search = dict.getDict(0).find("CELL" + Config::sep[0] + text);
		if(search != dict.getDict(0).end())
		{
			line.erase(esm.getScptTextPos(i), esm.getScptText(i).size() + 2);
			line.insert(esm.getScptTextPos(i), "\"" + search->second + "\"");
			counter++;
			found = 1;
		}
		if(found == 0)
		{
			for(auto &elem : dict.getDict(0))
			{
				if(caseInsensitiveStringCmp("CELL" + Config::sep[0] + text, elem.first) == 1)
				{
					line.erase(esm.getScptTextPos(i), esm.getScptText(i).size() + 2);
					line.insert(esm.getScptTextPos(i), "\"" + elem.second + "\"");
					counter++;
					found = 1;
					break;
				}
			}
		}
	}
	if(found == 1)
	{
		script_text += line + "\r\n";
	}
	else
	{
		script_text += esm.getScptLine(i) + "\r\n";
	}
}

//----------------------------------------------------------
void EsmConverter::convertCELL()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "CELL")
		{
			esm.setPri("NAME");
			pri_text = "CELL" + Config::sep[0] + esm.getPriText();
			auto search = dict.getDict(0).find(pri_text);

			if(search != dict.getDict(0).end() &&
			   esm.getPriText() != search->second &&
			   !esm.getPriText().empty())
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> CELL records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertPGRD()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "PGRD")
		{
			esm.setPri("NAME");
			pri_text = "CELL" + Config::sep[0] + esm.getPriText();
			auto search = dict.getDict(0).find(pri_text);

			if(search != dict.getDict(0).end() &&
			   esm.getPriText() != search->second)
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> PGRD records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertANAM()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setPri("ANAM");
			pri_text = "CELL" + Config::sep[0] + esm.getPriText();
			auto search = dict.getDict(0).find(pri_text);

			if(search != dict.getDict(0).end() &&
			   esm.getPriText() != search->second)
			{

				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> ANAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertSCVR()
{
	counter = 0;
	string text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setPriColl("SCVR");
			for(size_t j = 0; j < esm.getPriCollSize(); j++)
			{
				if(!esm.getPriText(j).empty() &&
				   esm.getPriText(j).substr(1, 1) == "B")
				{
					auto search = dict.getDict(0).find("CELL" + Config::sep[0] +
									   esm.getPriText(j).substr(5));

					if(search != dict.getDict(0).end() &&
					   esm.getPriText(j).substr(5) != search->second)
					{
						text = esm.getPriText(j).substr(0, 5) + search->second;
						convertRecordContent(esm.getPriPos(j),
								     esm.getPriSize(j),
								     text,
								     text.size(),
								     i);
						counter++;
						esm.setPriColl("SCVR");
					}
				}
			}
		}
	}
	Config::appendLog("    --> SCVR records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertDNAM()
{
	counter = 0;
	string text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "CELL" ||
		   esm.getRecId() == "NPC_")
		{
			esm.setPriColl("DNAM");
			for(size_t j = 0; j < esm.getPriCollSize(); j++)
			{
				if(!esm.getPriText(j).empty())
				{
					auto search = dict.getDict(0).find("CELL" + Config::sep[0] +
									   esm.getPriText(j));

					if(search != dict.getDict(0).end() &&
					   esm.getPriText(j) != search->second)
					{
						convertRecordContent(esm.getPriPos(j),
								     esm.getPriSize(j),
								     search->second + '\0',
								     search->second.size() + 1,
								     i);
						counter++;
						esm.setPriColl("DNAM");
					}
				}
			}
		}
	}
	Config::appendLog("    --> DNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertCNDT()
{
	counter = 0;
	string text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "NPC_")
		{
			esm.setPriColl("CNDT");
			for(size_t j = 0; j < esm.getPriCollSize(); j++)
			{
				if(!esm.getPriText(j).empty())
				{
					auto search = dict.getDict(0).find("CELL" + Config::sep[0] +
									   esm.getPriText(j));

					if(search != dict.getDict(0).end() &&
					   esm.getPriText() != search->second)
					{
						convertRecordContent(esm.getPriPos(j),
								     esm.getPriSize(j),
								     search->second + '\0',
								     search->second.size() + 1,
								     i);
						counter++;
						esm.setPriColl("CNDT");
					}
				}
			}
		}
	}
	Config::appendLog("    --> CNDT records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertGMST()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "GMST")
		{
			esm.setPri("NAME");
			esm.setSec("STRV");
			auto search = dict.getDict(1).find(esm.getRecId() + Config::sep[0] +
							   esm.getPriText());

			if(search != dict.getDict(1).end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> GMST records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertFNAM()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
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
			esm.setPri("NAME");
			esm.setSec("FNAM");
			auto search = dict.getDict(2).find(esm.getSecId() + Config::sep[0] +
							   esm.getRecId() + Config::sep[0] +
							   esm.getPriText());

			if(search != dict.getDict(2).end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> FNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertDESC()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BSGN" ||
		   esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE")
		{
			esm.setPri("NAME");
			esm.setSec("DESC");
			auto search = dict.getDict(3).find(esm.getSecId() + Config::sep[0] +
							  esm.getRecId() + Config::sep[0] +
							  esm.getPriText());

			if(search != dict.getDict(3).end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> DESC records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertTEXT()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BOOK")
		{
			esm.setPri("NAME");
			esm.setSec("TEXT");
			auto search = dict.getDict(4).find(esm.getSecId() + Config::sep[0] +
							   esm.getPriText());

			if(search != dict.getDict(4).end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> TEXT records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertRNAM()
{
	counter = 0;
	string text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "FACT")
		{
			esm.setPri("NAME");
			esm.setSecColl("RNAM");
			for(size_t j = 0; j < esm.getSecCollSize(); j++)
			{
				auto search = dict.getDict(5).find(esm.getSecId() + Config::sep[0] +
								   esm.getPriText() + Config::sep[0] +
								   to_string(j));

				if(search != dict.getDict(5).end() &&
				   esm.getSecText() != search->second)
				{
					text = search->second;
					text.resize(32);
					convertRecordContent(esm.getSecPos(j),
							     32,
							     text,
							     32,
							     i);
					counter++;
					esm.setSecColl("RNAM");
				}
			}
		}
	}
	Config::appendLog("    --> RNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertINDX()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setPriINDX();
			esm.setSec("DESC");
			auto search = dict.getDict(6).find(esm.getPriId() + Config::sep[0] +
							   esm.getRecId() + Config::sep[0] +
							   esm.getPriText());

			if(search != dict.getDict(6).end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> INDX records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertDIAL()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setPri("NAME");
			esm.setSec("DATA");
			auto search = dict.getDict(7).find(esm.getRecId() + Config::sep[0] +
							   esm.getPriText());

			if(search != dict.getDict(7).end() &&
			   esm.getPriText() != search->second)
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> DIAL records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
	counter = 0;
	string dial;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setPri("NAME");
			esm.setSec("DATA");
			dial = esm.getDialType() + Config::sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setPri("INAM");
			esm.setSec("NAME");
			auto search = dict.getDict(8).find(esm.getRecId() + Config::sep[0] +
							   dial + Config::sep[0] +
							   esm.getPriText());

			if(search != dict.getDict(8).end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1,
						     i);
				counter++;
			}
		}
	}
	Config::appendLog("    --> INFO records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setPri("INAM");
			esm.setSec("BNAM");
			esm.setScptColl("BNAM", esm.getSecText());
			script_text.erase();
			if(esm.getScptCollSize() > 0)
			{
				for(size_t k = 0; k < esm.getScptCollSize(); k++)
				{
					convertScriptLine(k);
				}
				script_text.erase(script_text.size() - 2);
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     script_text,
						     script_text.size(),
						     i);
			}
		}
	}
	Config::appendLog("    --> BNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertSCPT()
{
	counter = 0;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "SCPT")
		{
			esm.setPri("SCHD");
			esm.setSec("SCTX");
			esm.setScptColl("SCPT", esm.getSecText());
			script_text.erase();
			if(esm.getScptCollSize() > 0)
			{
				for(size_t k = 0; k < esm.getScptCollSize(); k++)
				{
					convertScriptLine(k);
				}
				script_text.erase(script_text.size() - 2);
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     script_text,
						     script_text.size(),
						     i);
			}
		}
	}
	Config::appendLog("    --> SCPT records converted: " + to_string(counter) + "\r\n");
}
