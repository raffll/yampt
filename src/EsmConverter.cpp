#include "EsmConverter.hpp"

using namespace std;

//----------------------------------------------------------
EsmConverter::EsmConverter(string path, DictMerger &m)
{
	esm.readFile(path);
	merger = m;
	if(esm.getStatus() == 1 && merger.getStatus() == 1)
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
	}
}

//----------------------------------------------------------
void EsmConverter::convertEsmWithDial()
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
		convertINFOWithDIAL();
		convertBNAM();
		convertSCPT();
	}
}
//----------------------------------------------------------
void EsmConverter::writeEsm()
{
	if(status == 1)
	{
		string name = esm.getNamePrefix() + Config::output_suffix + esm.getNameSuffix();
		ofstream file(name, ios::binary);
		for(auto &elem : esm.getRecColl())
		{
			file << elem;
		}
		cout << "--> Writing " << name << "...\r\n";
	}
}

//----------------------------------------------------------
string EsmConverter::convertIntToByteArray(unsigned int x)
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
					size_t new_size)
{
	unsigned int rec_size;
	rec_content = esm.getRecContent();
	rec_content.erase(pos + 8, old_size);
	rec_content.insert(pos + 8, new_text);
	rec_content.erase(pos + 4, 4);
	rec_content.insert(pos + 4, convertIntToByteArray(new_size));
	rec_size = rec_content.size() - 16;
	rec_content.erase(4, 4);
	rec_content.insert(4, convertIntToByteArray(rec_size));
	esm.setRecContent(rec_content);
}

//----------------------------------------------------------
void EsmConverter::convertScriptLine(size_t i)
{
	bool found = 0;
	string line = esm.getScptLine(i);
	string text = esm.getScptText(i);
	if(esm.getScptLineType(i) == "BNAM")
	{
		auto search = merger.getDict()[RecType::BNAM].find("BNAM" + sep[0] + esm.getScptLine(i));
		if(search != merger.getDict()[RecType::BNAM].end())
		{
			line = search->second;
			line = line.substr(line.find(sep[0]) + 1);
			found = 1;
			if(esm.getScptLine(i) != line)
			{
				counter++;
			}
		}
	}
	else if(esm.getScptLineType(i) == "SCTX")
	{
		auto search = merger.getDict()[RecType::SCTX].find("SCTX" + sep[0] + esm.getScptLine(i));
		if(search != merger.getDict()[RecType::SCTX].end())
		{
			line = search->second;
			line = line.substr(line.find(sep[0]) + 1);
			found = 1;
			if(esm.getScptLine(i) != line)
			{
				counter++;
			}
		}
	}
	else if(esm.getScptLineType(i) == "DIAL")
	{
		auto search = merger.getDict()[RecType::DIAL].find("DIAL" + sep[0] + text);
		if(search != merger.getDict()[RecType::DIAL].end())
		{

			line.erase(esm.getScptTextPos(i), esm.getScptText(i).size() + 2);
			line.insert(esm.getScptTextPos(i), "\"" + search->second + "\"");
			counter++;
			found = 1;
		}
		if(found == 0)
		{
			for(auto &elem : merger.getDict()[RecType::DIAL])
			{
				if(caseInsensitiveStringCmp("DIAL" + sep[0] + text, elem.first) == 1)
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
		auto search = merger.getDict()[RecType::CELL].find("CELL" + sep[0] + text);
		if(search != merger.getDict()[RecType::CELL].end())
		{
			line.erase(esm.getScptTextPos(i), esm.getScptText(i).size() + 2);
			line.insert(esm.getScptTextPos(i), "\"" + search->second + "\"");
			counter++;
			found = 1;
		}
		if(found == 0)
		{
			for(auto &elem : merger.getDict()[RecType::CELL])
			{
				if(caseInsensitiveStringCmp("CELL" + sep[0] + text, elem.first) == 1)
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
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "CELL")
		{
			esm.setPri("NAME");

			pri_text = "CELL" + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::CELL].find(pri_text);

			if(search != merger.getDict()[RecType::CELL].end() &&
			   esm.getPriText() != search->second &&
			   !esm.getPriText().empty())
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> CELL records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertPGRD()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "PGRD")
		{
			esm.setPri("NAME");

			pri_text = "CELL" + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::CELL].find(pri_text);

			if(search != merger.getDict()[RecType::CELL].end() &&
			   esm.getPriText() != search->second)
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> PGRD records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertANAM()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setPri("ANAM");

			pri_text = "CELL" + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::CELL].find(pri_text);

			if(search != merger.getDict()[RecType::CELL].end() &&
			   esm.getPriText() != search->second)
			{

				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> ANAM records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertSCVR()
{
	counter = 0;
	string pri_text;
	string scvr_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setPriColl("SCVR");
			for(size_t k = 0; k < esm.getPriColl().size(); ++k)
			{
				if(!esm.getPriText(k).empty() &&
				   esm.getPriText(k).substr(1, 1) == "B")
				{
					pri_text = "CELL" + sep[0] + esm.getPriText(k).substr(5);
					auto search = merger.getDict()[RecType::CELL].find(pri_text);

					if(search != merger.getDict()[RecType::CELL].end() &&
					   esm.getPriText(k).substr(5) != search->second)
					{
						scvr_text = esm.getPriText(k).substr(0, 5) + search->second;
						convertRecordContent(esm.getPriPos(k),
								     esm.getPriSize(k),
								     scvr_text,
								     scvr_text.size());
						counter++;
						esm.setPriColl("SCVR");
					}
				}
			}
		}
	}
	cout << "    --> SCVR records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertDNAM()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "CELL" ||
		   esm.getRecId() == "NPC_")
		{
			esm.setPriColl("DNAM");
			for(size_t k = 0; k < esm.getPriColl().size(); ++k)
			{
				if(!esm.getPriText(k).empty())
				{
					pri_text = "CELL" + sep[0] + esm.getPriText(k);
					auto search = merger.getDict()[RecType::CELL].find(pri_text);

					if(search != merger.getDict()[RecType::CELL].end() &&
					   esm.getPriText(k) != search->second)
					{
						convertRecordContent(esm.getPriPos(k),
								     esm.getPriSize(k),
								     search->second + '\0',
								     search->second.size() + 1);
						counter++;
						esm.setPriColl("DNAM");
					}
				}
			}
		}
	}
	cout << "    --> DNAM records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertCNDT()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "NPC_")
		{
			esm.setPriColl("CNDT");
			for(size_t k = 0; k < esm.getPriColl().size(); ++k)
			{
				if(!esm.getPriText(k).empty())
				{
					pri_text = "CELL" + sep[0] + esm.getPriText(k);
					auto search = merger.getDict()[RecType::CELL].find(pri_text);

					if(search != merger.getDict()[RecType::CELL].end() &&
					   esm.getPriText() != search->second)
					{
						convertRecordContent(esm.getPriPos(k),
								     esm.getPriSize(k),
								     search->second + '\0',
								     search->second.size() + 1);
						counter++;
						esm.setPriColl("CNDT");
					}
				}
			}
		}
	}
	cout << "    --> CNDT records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertGMST()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "GMST")
		{
			esm.setPri("NAME");
			esm.setSec("STRV");

			pri_text = "GMST" + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::GMST].find(pri_text);

			if(search != merger.getDict()[RecType::GMST].end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> GMST records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertFNAM()
{
	counter = 0;
	string pri_text;
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

			pri_text = "FNAM" + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::FNAM].find(pri_text);

			if(search != merger.getDict()[RecType::FNAM].end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> FNAM records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertDESC()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BSGN" ||
		   esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE")
		{
			esm.setPri("NAME");
			esm.setSec("DESC");

			pri_text = "DESC" + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::DESC].find(pri_text);

			if(search != merger.getDict()[RecType::DESC].end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> DESC records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertTEXT()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BOOK")
		{
			esm.setPri("NAME");
			esm.setSec("TEXT");

			pri_text = "TEXT" + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::TEXT].find(pri_text);

			if(search != merger.getDict()[RecType::TEXT].end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> TEXT records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertRNAM()
{
	counter = 0;
	string pri_text;
	string rnam_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "FACT")
		{
			esm.setPri("NAME");
			esm.setSecColl("RNAM");
			for(size_t k = 0; k < esm.getSecColl().size(); ++k)
			{
				pri_text = "RNAM" + sep[0] + esm.getPriText() + sep[0] + to_string(k);
				auto search = merger.getDict()[RecType::RNAM].find(pri_text);

				if(search != merger.getDict()[RecType::RNAM].end() &&
				   esm.getSecText() != search->second)
				{
					rnam_text = search->second;
					rnam_text.resize(32);
					convertRecordContent(esm.getSecPos(k),
							     32,
							     rnam_text,
							     32);
					counter++;
					esm.setSecColl("RNAM");
				}
			}
		}
	}
	cout << "    --> RNAM records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertINDX()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setPriINDX();
			esm.setSec("DESC");

			pri_text = "INDX" + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::INDX].find(pri_text);

			if(search != merger.getDict()[RecType::INDX].end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> INDX records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertDIAL()
{
	counter = 0;
	string pri_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setPri("NAME");
			esm.setSecDialType("DATA");

			pri_text = "DIAL" + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::DIAL].find(pri_text);

			if(search != merger.getDict()[RecType::DIAL].end() &&
			   esm.getPriText() != search->second)
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> DIAL records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
	counter = 0;
	string pri_text;
	string dial;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setPri("NAME");
			esm.setSecDialType("DATA");
			dial = esm.getDialType() + sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setPri("INAM");
			esm.setSec("NAME");

			pri_text = "INFO" + sep[0] + dial + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::INFO].find(pri_text);

			if(search != merger.getDict()[RecType::INFO].end() &&
			   esm.getSecText() != search->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> INFO records converted: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertINFOWithDIAL()
{
	counter = 0;
	string pri_text;
	string sec_text;
	size_t pos;
	string dial;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setPri("NAME");
			esm.setSecDialType("DATA");
			dial = esm.getDialType() + sep[0] + esm.getPriText();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setPri("INAM");
			esm.setSec("NAME");

			pri_text = "INFO" + sep[0] + dial + sep[0] + esm.getPriText();
			auto search = merger.getDict()[RecType::INFO].find(pri_text);

			if(search != merger.getDict()[RecType::INFO].end())
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search->second + '\0',
						     search->second.size() + 1);
				counter++;
			}
			else if(!esm.getSecText().empty())
			{
				sec_text = esm.getSecText();
				for(auto &elem : merger.getDict()[RecType::DIAL])
				{
					if(elem.first.substr(5) != elem.second)
					{
						string r = "\\b" + elem.first.substr(5) + "\\b";
						regex re(r, regex_constants::icase);
						smatch found;
						regex_search(sec_text, found, re);
						if(!found[0].str().empty())
						{
							pos = found.position(0) + found[0].str().size();
							sec_text.insert(pos, " [" + elem.second + "]");
						}
					}
				}
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     sec_text + '\0',
						     sec_text.size() + 1);
				counter++;
			}
		}
	}
	cout << "    --> INFO records converted: " << to_string(counter) << "\r\n";
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
			esm.setSecScptColl("BNAM");

			script_text.erase();
			if(esm.getScptColl().size() > 0)
			{
				for(size_t k = 0; k < esm.getScptColl().size(); ++k)
				{
					convertScriptLine(k);
				}
				script_text.erase(script_text.size() - 2);
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     script_text,
						     script_text.size());
			}
		}
	}
	cout << "    --> BNAM records converted: " << to_string(counter) << "\r\n";
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
			esm.setSecScptColl("SCTX");

			script_text.erase();
			if(esm.getScptColl().size() > 0)
			{
				for(size_t k = 0; k < esm.getScptColl().size(); ++k)
				{
					convertScriptLine(k);
				}
				script_text.erase(script_text.size() - 2);
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     script_text,
						     script_text.size());
			}
		}
	}
	cout << "    --> SCTX records converted: " << to_string(counter) << "\r\n";
}
