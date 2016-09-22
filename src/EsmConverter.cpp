#include "EsmConverter.hpp"

using namespace std;

//----------------------------------------------------------
EsmConverter::EsmConverter()
{

}

//----------------------------------------------------------
EsmConverter::EsmConverter(string path, DictMerger &n)
{
	esm.readFile(path);
	merger = n;
	if(esm.getStatus() == true && merger.getStatus() == true)
	{
		status = true;
	}
}

//----------------------------------------------------------
void EsmConverter::convertEsm()
{
	if(status == true)
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
void EsmConverter::writeEsm()
{
	if(status == true)
	{
		string name = esm.getName();
		ofstream file(name, ios::binary);
		for(auto &elem : esm.getRecColl())
		{
			file << elem;
		}
		cout << "--> Writing " << name << "...\r\n";
	}
}

void EsmConverter::setConditions(const std::string &pri_text_n,
				 const std::string &pri_text_f,
				 const std::string &sec_text,
				 RecType type)
{
	found_n = false;
	found_f = false;
	equal_n = false;
	equal_f = false;

	auto search_n = merger.getDict()[type].find(pri_text_n);
	auto search_f = merger.getDict()[type].find(pri_text_f);

	if(search_n != merger.getDict()[type].end())
	{
		found_n = true;
		if(sec_text == search_n->second)
		{
			equal_n = true;
		}
	}

	if(search_f != merger.getDict()[type].end())
	{
		found_f = true;
		if(sec_text == search_f->second)
		{
			equal_f = true;
		}
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
		return true;
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent(size_t pos, size_t old_size, string new_text,
					size_t new_size)
{
	size_t rec_size;
	rec_content = esm.getRecContent();

	if(pos < rec_content.size() && pos >= 16)
	{
		rec_content.erase(pos + 8, old_size);
		rec_content.insert(pos + 8, new_text);
		rec_content.erase(pos + 4, 4);
		rec_content.insert(pos + 4, convertIntToByteArray(new_size));
		rec_size = rec_content.size() - 16;
		rec_content.erase(4, 4);
		rec_content.insert(4, convertIntToByteArray(rec_size));
		esm.setRecContent(rec_content);
	}
}

//----------------------------------------------------------
void EsmConverter::convertScriptLine(size_t i)
{
	bool found = false;
	string line = esm.getScptLine(i);
	string text = esm.getScptText(i);
	if(esm.getScptLineType(i) == "BNAM")
	{
		auto search_n = merger.getDict()[RecType::BNAM].find("BNAM" + sep[0] + esm.getScptLine(i));
		if(search_n != merger.getDict()[RecType::BNAM].end())
		{
			line = search_n->second;
			line = line.substr(line.find(sep[0]) + 1);
			if(esm.getScptLine(i) != line)
			{
				found = true;
				counter_message++;
			}
		}
	}
	else if(esm.getScptLineType(i) == "SCTX")
	{
		auto search_n = merger.getDict()[RecType::SCTX].find("SCTX" + sep[0] + esm.getScptLine(i));
		if(search_n != merger.getDict()[RecType::SCTX].end())
		{
			line = search_n->second;
			line = line.substr(line.find(sep[0]) + 1);
			if(esm.getScptLine(i) != line)
			{
				found = true;
				counter_message++;
			}
		}
	}
	else if(esm.getScptLineType(i) == "DIAL")
	{
		auto search_n = merger.getDict()[RecType::DIAL].find("DIAL" + sep[0] + text);
		if(search_n != merger.getDict()[RecType::DIAL].end())
		{
			if(text != search_n->second)
			{
				line.erase(esm.getScptTextPos(i), text.size() + 2);
				line.insert(esm.getScptTextPos(i), "\"" + search_n->second + "\"");
				found = true;
				counter_dial++;
			}
		}
		if(found == false)
		{
			for(auto &elem : merger.getDict()[RecType::DIAL])
			{
				if(caseInsensitiveStringCmp("DIAL" + sep[0] + text, elem.first) == 1)
				{
					line.erase(esm.getScptTextPos(i), text.size() + 2);
					line.insert(esm.getScptTextPos(i), "\"" + elem.second + "\"");
					found = true;
					counter_dial++;
					break;
				}
			}
		}
	}
	else if(esm.getScptLineType(i) == "CELL")
	{
		auto search_n = merger.getDict()[RecType::CELL].find("CELL" + sep[0] + text);
		if(search_n != merger.getDict()[RecType::CELL].end())
		{
			if(text != search_n->second)
			{
				line.erase(esm.getScptTextPos(i), text.size() + 2);
				line.insert(esm.getScptTextPos(i), "\"" + search_n->second + "\"");
				found = true;
				counter_cell++;
			}
		}
		if(found == false)
		{
			for(auto &elem : merger.getDict()[RecType::CELL])
			{
				if(caseInsensitiveStringCmp("CELL" + sep[0] + text, elem.first) == 1)
				{
					line.erase(esm.getScptTextPos(i), text.size() + 2);
					line.insert(esm.getScptTextPos(i), "\"" + elem.second + "\"");
					found = true;
					counter_cell++;
					break;
				}
			}
		}
	}
	if(found == true)
	{
		script_text += line + "\r\n";
		counter++;
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
			auto search_n = merger.getDict()[RecType::CELL].find(pri_text);

			if(search_n != merger.getDict()[RecType::CELL].end() &&
			   esm.getPriText() != search_n->second &&
			   esm.getPriStatus())
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
			auto search_n = merger.getDict()[RecType::CELL].find(pri_text);

			if(search_n != merger.getDict()[RecType::CELL].end() &&
			   esm.getPriText() != search_n->second)
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
			auto search_n = merger.getDict()[RecType::CELL].find(pri_text);

			if(search_n != merger.getDict()[RecType::CELL].end() &&
			   esm.getPriText() != search_n->second)
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
	string sec_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setPriColl("SCVR");
			for(size_t k = 0; k < esm.getPriColl().size(); ++k)
			{
				if(esm.getPriStatus(k) &&
				   esm.getPriText(k).substr(1, 1) == "B")
				{
					pri_text = "CELL" + sep[0] + esm.getPriText(k).substr(5);
					auto search_n = merger.getDict()[RecType::CELL].find(pri_text);

					if(search_n != merger.getDict()[RecType::CELL].end() &&
					   esm.getPriText(k).substr(5) != search_n->second)
					{
						sec_text = esm.getPriText(k).substr(0, 5) + search_n->second;
						convertRecordContent(esm.getPriPos(k),
								     esm.getPriSize(k),
								     sec_text,
								     sec_text.size());
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
				if(esm.getPriStatus(k))
				{
					pri_text = "CELL" + sep[0] + esm.getPriText(k);
					auto search_n = merger.getDict()[RecType::CELL].find(pri_text);

					if(search_n != merger.getDict()[RecType::CELL].end() &&
					   esm.getPriText(k) != search_n->second)
					{
						convertRecordContent(esm.getPriPos(k),
								     esm.getPriSize(k),
								     search_n->second + '\0',
								     search_n->second.size() + 1);
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
				if(esm.getPriStatus(k))
				{
					pri_text = "CELL" + sep[0] + esm.getPriText(k);
					auto search_n = merger.getDict()[RecType::CELL].find(pri_text);

					if(search_n != merger.getDict()[RecType::CELL].end() &&
					   esm.getPriText() != search_n->second)
					{
						convertRecordContent(esm.getPriPos(k),
								     esm.getPriSize(k),
								     search_n->second + '\0',
								     search_n->second.size() + 1);
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
	counter_safe = 0;
	string pri_text_n;
	string pri_text_f;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "GMST")
		{
			esm.setPri("NAME");
			esm.setSec("STRV");

			pri_text_n = "GMST" + sep[0] + esm.getPriText();
			pri_text_f = "GMST" + ext + sep[0] + esm.getPriText();
			auto search_n = merger.getDict()[RecType::GMST].find(pri_text_n);
			auto search_f = merger.getDict()[RecType::GMST].find(pri_text_f);

			if(Config::getSafeConvert() == false &&
			   search_n != merger.getDict()[RecType::GMST].end() &&
			   esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
				counter++;
			}
			else if(Config::getSafeConvert() == true &&
				search_f != merger.getDict()[RecType::GMST].end() &&
				search_n != merger.getDict()[RecType::GMST].end() &&
				esm.getSecText() == search_f->second &&
				esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
	string pri_text_n;
	string pri_text_f;
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

			pri_text_n = "FNAM" + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			pri_text_f = "FNAM" + ext + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			auto search_n = merger.getDict()[RecType::FNAM].find(pri_text_n);
			auto search_f = merger.getDict()[RecType::FNAM].find(pri_text_f);

			if(Config::getSafeConvert() == false &&
			   search_n != merger.getDict()[RecType::FNAM].end() &&
			   esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
				counter++;
			}
			else if(Config::getSafeConvert() == true &&
				search_f != merger.getDict()[RecType::FNAM].end() &&
				search_n != merger.getDict()[RecType::FNAM].end() &&
				esm.getSecText() == search_f->second &&
				esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
	string pri_text_n;
	string pri_text_f;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BSGN" ||
		   esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE")
		{
			esm.setPri("NAME");
			esm.setSec("DESC");

			pri_text_n = "DESC" + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			pri_text_f = "DESC" + ext + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			auto search_n = merger.getDict()[RecType::DESC].find(pri_text_n);
			auto search_f = merger.getDict()[RecType::DESC].find(pri_text_f);

			if(Config::getSafeConvert() == false &&
			   search_n != merger.getDict()[RecType::DESC].end() &&
			   esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
				counter++;
			}
			else if(Config::getSafeConvert() == true &&
				search_f != merger.getDict()[RecType::DESC].end() &&
				search_n != merger.getDict()[RecType::DESC].end() &&
				esm.getSecText() == search_f->second &&
				esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
	string pri_text_n;
	string pri_text_f;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BOOK")
		{
			esm.setPri("NAME");
			esm.setSec("TEXT");

			pri_text_n = "TEXT" + sep[0] + esm.getPriText();
			pri_text_f = "TEXT" + ext + sep[0] + esm.getPriText();
			auto search_n = merger.getDict()[RecType::TEXT].find(pri_text_n);
			auto search_f = merger.getDict()[RecType::TEXT].find(pri_text_f);

			if(Config::getSafeConvert() == false &&
			   search_n != merger.getDict()[RecType::TEXT].end() &&
			   esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
				counter++;
			}
			else if(Config::getSafeConvert() == true &&
				search_f != merger.getDict()[RecType::TEXT].end() &&
				search_n != merger.getDict()[RecType::TEXT].end() &&
				esm.getSecText() == search_f->second &&
				esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
	string pri_text_n;
	string pri_text_f;
	string sec_text;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "FACT")
		{
			esm.setPri("NAME");
			esm.setSecColl("RNAM");
			for(size_t k = 0; k < esm.getSecColl().size(); ++k)
			{
				pri_text_n = "RNAM" + sep[0] + esm.getPriText() + sep[0] + to_string(k);
				pri_text_f = "RNAM" + ext + sep[0] + esm.getPriText() + sep[0] + to_string(k);
				auto search_n = merger.getDict()[RecType::RNAM].find(pri_text_n);
				auto search_f = merger.getDict()[RecType::RNAM].find(pri_text_f);

				if(Config::getSafeConvert() == false &&
				   search_n != merger.getDict()[RecType::RNAM].end() &&
				   esm.getSecText(k) != search_n->second)
				{
					sec_text = search_n->second;
					sec_text.resize(32);
					convertRecordContent(esm.getSecPos(k),
							     esm.getSecSize(k),
							     sec_text,
							     32);
					counter++;
					esm.setSecColl("RNAM");
				}
				else if(Config::getSafeConvert() == true &&
					search_f != merger.getDict()[RecType::RNAM].end() &&
					search_n != merger.getDict()[RecType::RNAM].end() &&
					esm.getSecText(k) == search_f->second &&
					esm.getSecText(k) != search_n->second)
				{
					sec_text = search_n->second;
					sec_text.resize(32);
					convertRecordContent(esm.getSecPos(k),
							     esm.getSecSize(k),
							     sec_text,
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
	string pri_text_n;
	string pri_text_f;
	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setPriINDX();
			esm.setSec("DESC");

			pri_text_n = "INDX" + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			pri_text_f = "INDX" + ext + sep[0] + esm.getRecId() + sep[0] + esm.getPriText();
			auto search_n = merger.getDict()[RecType::INDX].find(pri_text_n);
			auto search_f = merger.getDict()[RecType::INDX].find(pri_text_f);

			if(Config::getSafeConvert() == false &&
			   search_n != merger.getDict()[RecType::INDX].end() &&
			   esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
				counter++;
			}
			else if(Config::getSafeConvert() == true &&
				search_f != merger.getDict()[RecType::INDX].end() &&
				search_n != merger.getDict()[RecType::INDX].end() &&
				esm.getSecText() == search_f->second &&
				esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
			auto search_n = merger.getDict()[RecType::DIAL].find(pri_text);

			if(search_n != merger.getDict()[RecType::DIAL].end() &&
			   esm.getPriText() != search_n->second)
			{
				convertRecordContent(esm.getPriPos(),
						     esm.getPriSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
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
	counter_add = 0;
	string pri_text_n;
	string pri_text_f;
	string sec_text;
	string sec_text_lowercase;
	size_t pos;
	string text;
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

			pri_text_n = "INFO" + sep[0] + dial + sep[0] + esm.getPriText();
			pri_text_f = "INFO" + ext + sep[0] + dial + sep[0] + esm.getPriText();
			auto search_n = merger.getDict()[RecType::INFO].find(pri_text_n);
			auto search_f = merger.getDict()[RecType::INFO].find(pri_text_f);

			if(Config::getSafeConvert() == false &&
			   search_n != merger.getDict()[RecType::INFO].end() &&
			   esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
				counter++;
			}
			else if(Config::getSafeConvert() == true &&
				search_f != merger.getDict()[RecType::INFO].end() &&
				search_n != merger.getDict()[RecType::INFO].end() &&
				esm.getSecText() == search_f->second &&
				esm.getSecText() != search_n->second)
			{
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     search_n->second + '\0',
						     search_n->second.size() + 1);
				counter++;
			}
			else if(Config::getAddDialToInfo() == true &&
				!esm.getSecText().empty() &&
				dial.substr(0, 1) != "V")
			{
				sec_text = esm.getSecText();
				for(auto &elem : merger.getDict()[RecType::DIAL])
				{
				        text = elem.first.substr(5);
					if(text != elem.second)
					{
						sec_text_lowercase = sec_text;
						transform(sec_text_lowercase.begin(), sec_text_lowercase.end(),
							  sec_text_lowercase.begin(), ::tolower);
						transform(text.begin(), text.end(),
							  text.begin(), ::tolower);
						pos = sec_text_lowercase.find(text);
						if(pos != string::npos)
						{
							sec_text.insert(sec_text.size(), " [" + elem.second + "]");
						}
					}
				}
				convertRecordContent(esm.getSecPos(),
						     esm.getSecSize(),
						     sec_text + '\0',
						     sec_text.size() + 1);
				counter_add++;
			}
		}
	}
	cout << "    --> INFO records converted: " << to_string(counter) << "\r\n";
	if(counter_add > 0)
	{
		cout << "        --> INFO records with added dialog topic names: " << to_string(counter_add) << "\r\n";
	}
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
	counter = 0;
	counter_message = 0;
	counter_dial = 0;
	counter_cell = 0;
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
	cout << "    --> BNAM lines converted: " << to_string(counter) << "\r\n";
	cout << "        --> Message: " << to_string(counter_message) << "\r\n";
	cout << "        --> DIAL   : " << to_string(counter_dial) << "\r\n";
	cout << "        --> CELL   : " << to_string(counter_cell) << "\r\n";
}

//----------------------------------------------------------
void EsmConverter::convertSCPT()
{
	counter = 0;
	counter_message = 0;
	counter_dial = 0;
	counter_cell = 0;
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
	cout << "    --> SCTX lines converted: " << to_string(counter) << "\r\n";
	cout << "        --> Message: " << to_string(counter_message) << "\r\n";
	cout << "        --> DIAL   : " << to_string(counter_dial) << "\r\n";
	cout << "        --> CELL   : " << to_string(counter_cell) << "\r\n";
}
