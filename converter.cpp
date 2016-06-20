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
		convertCELL();
		convertPGRD();
		convertANAM();
		convertSCVR();
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
		cerr << "Converting complete!" << endl;
	}
}

//----------------------------------------------------------
void converter::writeEsm()
{
	if(status == 1)
	{
		string name = esm.getEsmPrefix() + "-Converted" + esm.getEsmSuffix();
		ofstream file(name, ios::binary);
		file << esm.getEsmContent();
		cerr << "Writing " << name << "..." << endl;
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
bool converter::caseInsensitiveStringCmp(string lhs, string rhs)
{
	transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);
	transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);
	if(lhs == rhs)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------------
void converter::printRecord()
{
	for(size_t i = 0; i < rec_content.size(); i++)
	{
		if(isprint(rec_content.at(i)))
		{
			cout << rec_content.at(i);
		}
		else
		{
			cout << ".";
		}
	}
}

//----------------------------------------------------------
void converter::convertRecordContent(size_t pos, size_t old_size,
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
void converter::convertScriptLine(int i, string id)
{
	bool found = 0;
	string line = esm.getCollText(i);
	string text = esm.getCollSubStr(i);
	if(esm.getCollKind(i) == esmtools::NOCHANGE)
	{
		script_text += esm.getCollText(i) + "\r\n";
	}
	else if(esm.getCollKind(i) == esmtools::MESSAGE)
	{
		if(id == "BNAM")
		{
			text = "BNAM" + sep[0] + text;
		}
		else
		{
			text = "SCPT" + sep[0] + text;
		}
		auto search = dict.getDict().find(id + sep[0] + esm.getCollText(i));
		if(search != dict.getDict().end())
		{
			string line = search->second;
			line = line.substr(line.find(sep[0]) + sep[0].size());
			script_text += line + "\r\n";
			counter++;
		}
		else
		{
			script_text += esm.getCollText(i) + "\r\n";
		}
	}
	else if(esm.getCollKind(i) == esmtools::DIAL || esm.getCollKind(i) == esmtools::CELL)
	{
		if(text.find("\"") == 0 && text.rfind("\"") == text.size() - 1)
		{
			text = text.substr(1, text.size() - 2);
		}
		if(esm.getCollKind(i) == esmtools::DIAL)
		{
			text = "DIAL" + sep[0] + text;
		}
		else
		{
			text = "CELL" + sep[0] + text;
		}
		for(auto &elem : dict.getDict())
		{
			if(caseInsensitiveStringCmp(text, elem.first) == 1)
			{
				line.erase(esm.getCollPos(i), esm.getCollSubStr(i).size());
				line.insert(esm.getCollPos(i), "\"" + elem.second + "\"");
				found = 1;
				break;
			}
		}
		if(found == 1)
		{
			script_text += line + "\r\n";
			counter++;
		}
		else
		{
			script_text += esm.getCollText(i) + "\r\n";
		}
	}
}

//----------------------------------------------------------
void converter::convertCELL()
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
			auto search = dict.getDict().find("CELL" + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertPGRD()
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
			auto search = dict.getDict().find("CELL" + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "PGRD records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertANAM()
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
			auto search = dict.getDict().find("CELL" + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
				}
				counter++;
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "ANAM records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertSCVR()
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
			esm.setSecSubRec("INAM");
			while(esm.setPriSubRec("SCVR", 4))
			{
				// TODO
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "SCVR records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertGMST()
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
			auto search = dict.getDict().find(esm.getRecId() + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertFNAM()
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
			auto search = dict.getDict().find(esm.getSecId() + sep[0] +
							  esm.getRecId() + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertDESC()
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
			auto search = dict.getDict().find(esm.getSecId() + sep[0] +
							  esm.getRecId() + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertTEXT()
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
			auto search = dict.getDict().find(esm.getSecId() + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertRNAM()
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
				auto search = dict.getDict().find(esm.getSecId() + sep[0] +
								  esm.getPriText() + sep[0] +
								  to_string(rnam) + sep[0] +
								  esm.getSecText());
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
					}
					rnam++;
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
void converter::convertINDX()
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
			auto search = dict.getDict().find(esm.getPriId() + sep[0] +
							  esm.getRecId() + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertDIAL()
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
			auto search = dict.getDict().find(esm.getRecId() + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getPriText() != search->second)
				{
					convertRecordContent(esm.getPriPos(),
							     esm.getPriSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertINFO()
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
			dial = esm.dialType() + sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("NAME");
			auto search = dict.getDict().find(esm.getRecId() + sep[0] +
							  dial + sep[0] +
							  esm.getPriText());
			if(search != dict.getDict().end())
			{
				if(esm.getSecText() != search->second)
				{
					convertRecordContent(esm.getSecPos(),
							     esm.getSecSize(),
							     search->second + '\0',
							     search->second.size() + 1);
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
void converter::convertBNAM()
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
	cerr << "BNAM records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertSCPT()
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
	cerr << "SCPT records converted: " << counter << endl;
}
