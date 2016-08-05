#include "converter.hpp"

using namespace std;

//----------------------------------------------------------
Converter::Converter(string esm_path, Merger &m)
{
	esm.readEsm(esm_path);
	dict = m;
	if(esm.getEsmStatus() == 1 && dict.getMergerStatus() == 1)
	{
		status = 1;
	}
}

//----------------------------------------------------------
void Converter::convertEsm()
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
void Converter::writeEsm()
{
	if(status == 1)
	{
		string name = esm.getEsmPrefix() + Config::output_suffix + esm.getEsmSuffix();
		ofstream file(Config::output_path + name, ios::binary);
		file << esm.getEsmContent();
		Config::appendLog("--> Writing " + Config::output_path + name + "...\r\n");
	}
}

//----------------------------------------------------------
string Converter::intToByte(unsigned int x)
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
bool Converter::caseInsensitiveStringCmp(string lhs, string rhs)
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
void Converter::convertRecordContent(size_t pos, size_t old_size,
				     string new_text, size_t new_size)
{
	unsigned int rec_size;
	rec_content.erase(pos + 8, old_size);
	rec_content.insert(pos + 8, new_text);
	rec_content.erase(pos + 4, 4);
	rec_content.insert(pos + 4, intToByte(new_size));
	rec_size = rec_content.size() - 16;
	rec_content.erase(4, 4);
	rec_content.insert(4, intToByte(rec_size));
}

//----------------------------------------------------------
void Converter::convertScriptLine(int i, string id)
{
	bool found = 0;
	string line = esm.getCollLine(i);
	string text = esm.getCollText(i);
	if(esm.getCollType(i) == "MESSAGE")
	{
		auto search = dict.getDict().find(id + Config::Config::sep[0] + esm.getCollLine(i));
		if(search != dict.getDict().end())
		{
			line = search->second;
			line = line.substr(line.find(Config::sep[0]) + Config::sep[0].size());
			found = 1;
			if(esm.getCollLine(i) != line)
			{
				counter++;
			}
		}
	}
	else if(esm.getCollType(i) == "DIAL" || esm.getCollType(i) == "CELL")
	{
		auto search = dict.getDict().find(esm.getCollType(i) + Config::sep[0] + text);
		if(search != dict.getDict().end())
		{
			if(esm.getCollText(i) != search->second)
			{
				line.erase(esm.getCollPos(i), esm.getCollText(i).size() + 2);
				line.insert(esm.getCollPos(i), "\"" + search->second + "\"");
				counter++;
			}
			found = 1;
		}
		if(found == 0)
		{
			for(auto &elem : dict.getDict())
			{
				if(caseInsensitiveStringCmp(esm.getCollType(i) + Config::sep[0] + text,
							    elem.first) == 1 &&
				   text != elem.second)
				{
					line.erase(esm.getCollPos(i), esm.getCollText(i).size() + 2);
					line.insert(esm.getCollPos(i), "\"" + elem.second + "\"");
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
		script_text += esm.getCollLine(i) + "\r\n";
	}
}

//----------------------------------------------------------
void Converter::convertCELL()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "CELL")
		{
			esm.setPriSubRec("NAME");
			auto search = dict.getDict().find("CELL" + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> CELL records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertPGRD()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "PGRD")
		{
			esm.setPriSubRec("NAME");
			auto search = dict.getDict().find("CELL" + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> PGRD records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertANAM()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("ANAM");
			auto search = dict.getDict().find("CELL" + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> ANAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertSCVR()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	string text;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "INFO")
		{
			while(esm.setPriSubRec("SCVR", 4))
			{
				if(!esm.getPriText().empty() && esm.getPriText().substr(1, 1) == "B")
				{
					auto search = dict.getDict().find("CELL" + Config::sep[0] +
									  esm.getPriText().substr(5));
					if(search != dict.getDict().end())
					{
						if(esm.getPriText().substr(5) != search->second)
						{
							text = esm.getPriText().substr(0, 5) + search->second;
							convertRecordContent(esm.getPriPos(),
									     esm.getPriSize(),
									     text,
									     text.size());
							esm.setRecContent(rec_content);
							counter++;
						}
					}
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> SCVR records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertDNAM()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	string text;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "CELL" ||
		   esm.getRecId() == "NPC_")
		{
			while(esm.setPriSubRec("DNAM", 4))
			{
				if(!esm.getPriText().empty())
				{
					auto search = dict.getDict().find("CELL" + Config::sep[0] +
									  esm.getPriText());
					if(search != dict.getDict().end())
					{
						if(esm.getPriText() != search->second)
						{
							convertRecordContent(esm.getPriPos(),
									     esm.getPriSize(),
									     search->second + '\0',
									     search->second.size() + 1);
							esm.setRecContent(rec_content);
							counter++;
						}
					}
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> DNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertCNDT()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	string text;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "NPC_")
		{
			while(esm.setPriSubRec("CNDT", 4))
			{
				if(!esm.getPriText().empty())
				{
					auto search = dict.getDict().find("CELL" + Config::sep[0] +
									  esm.getPriText());
					if(search != dict.getDict().end())
					{
						if(esm.getPriText() != search->second)
						{
							convertRecordContent(esm.getPriPos(),
									     esm.getPriSize(),
									     search->second + '\0',
									     search->second.size() + 1);
							esm.setRecContent(rec_content);
							counter++;
						}
					}
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> CNDT records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertGMST()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "GMST")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("STRV");
			auto search = dict.getDict().find(esm.getRecId() + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> GMST records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertFNAM()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
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
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("FNAM");
			auto search = dict.getDict().find(esm.getSecId() + Config::sep[0] +
							  esm.getRecId() + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> FNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertDESC()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "BSGN" ||
		   esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DESC");
			auto search = dict.getDict().find(esm.getSecId() + Config::sep[0] +
							  esm.getRecId() + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> DESC records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertTEXT()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "BOOK")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("TEXT");
			auto search = dict.getDict().find(esm.getSecId() + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> TEXT records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertRNAM()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	int rnam;
	string text;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "FACT")
		{
			esm.setPriSubRec("NAME");
			rnam = 0;
			while(esm.setSecSubRec("RNAM", 4))
			{
				auto search = dict.getDict().find(esm.getSecId() + Config::sep[0] +
								  esm.getPriText() + Config::sep[0] +
								  to_string(rnam));
				if(search != dict.getDict().end())
				{
					if(esm.getSecText() != search->second)
					{
						text = search->second;
						text.resize(32);
						convertRecordContent(esm.getSecPos(),
								     esm.getSecSize(),
								     text,
								     text.size());
						esm.setRecContent(rec_content);
						counter++;
					}

				}
				rnam++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> RNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertINDX()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setPriSubRecINDX();
			esm.setSecSubRec("DESC");
			auto search = dict.getDict().find(esm.getPriId() + Config::sep[0] +
							  esm.getRecId() + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> INDX records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertDIAL()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "DIAL")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			auto search = dict.getDict().find(esm.getRecId() + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> DIAL records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertINFO()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	string dial;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "DIAL")
		{
			esm.setPriSubRec("NAME");
			esm.setSecSubRec("DATA");
			dial = esm.dialType() + Config::sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("NAME");
			auto search = dict.getDict().find(esm.getRecId() + Config::sep[0] +
							  dial + Config::sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
					counter++;
				}
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> INFO records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertBNAM()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("BNAM");
			esm.setCollScript();
			script_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					convertScriptLine(i, "BNAM");
				}
				script_text.erase(script_text.size() - 2);
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     script_text,
						     script_text.size());
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> BNAM records converted: " + to_string(counter) + "\r\n");
}

//----------------------------------------------------------
void Converter::convertSCPT()
{
	rec_content.erase();
	esm_content.erase();
	counter = 0;
	esm.resetRec();
	while(esm.setNextRec())
	{
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "SCPT")
		{
			esm.setPriSubRec("SCHD");
			esm.setSecSubRec("SCTX");
			esm.setCollScript();
			script_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					convertScriptLine(i, "SCPT");
				}
				script_text.erase(script_text.size() - 2);
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     script_text,
						     script_text.size());
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	Config::appendLog("    --> SCPT records converted: " + to_string(counter) + "\r\n");
}
