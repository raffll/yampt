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
		convertDialInBnam();
		convertCellInBnam();
		convertScpt();
		convertDialInScpt();
		convertCellInScpt();
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
			esm.setColl(esmtools::RNAM);
			for(size_t i = 0; i < esm.getCollSize(); i++)
			{
				auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getPriText() +
								  sep[0] + to_string(i) + sep[0] + esm.getCollText(i));
				if(search != dict.getDict().end())
				{
					if(esm.getSecText() != search->second)
					{
						// subrecord text
						rnam_text = search->second;
						rnam_text.resize(32);
						rec_content.erase(esm.getCollTextPos(i) + 8, esm.getSecSize());
						rec_content.insert(esm.getCollTextPos(i) + 8, rnam_text);
						// subrecord size
						sec_size = 32;
						rec_content.erase(esm.getCollTextPos(i) + 4, 4);
						rec_content.insert(esm.getCollTextPos(i) + 4, intToByte(sec_size));
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
	string line;
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
			esm.setColl(esmtools::BNAM);
			sec_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					if(esm.getCollTextStatus(i) == false)
					{
						sec_text += esm.getCollText(i) + "\r\n";
					}
					else
					{
						auto search = dict.getDict().find(esm.getSecId() + sep[0] + esm.getCollText(i));
						if(search == dict.getDict().end())
						{
							sec_text += esm.getCollText(i) + "\r\n";
						}
						else
						{
							line = search->second;
							line = line.substr(line.find(sep[0]) + sep[0].size());
							sec_text += line + "\r\n";
							counter++;
						}
					}
				}
				sec_text.erase(sec_text.size() - 2);
				// subrecord text
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
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "BNAM script lines converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertDialInBnam()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string sec_text;
	string line;
	string text;
	bool found;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("BNAM");
			esm.setColl(esmtools::DIAL);
			sec_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					if(esm.getCollTextStatus(i) == false)
					{
						sec_text += esm.getCollText(i) + "\r\n";
					}
					else
					{
						text = esm.getCollText(i).substr(esm.getCollTextPos(i), esm.getCollTextSize(i));
						if(text.find("\"") == 0 && text.rfind("\"") == text.size() - 1)
						{
							text = text.substr(1, text.size() - 2);
						}
						text = "DIAL" + sep[0] + text;
						cout << text << endl;
						found = 0;
						for(auto &elem : dict.getDict())
						{
							if(caseInsensitiveStringCmp(text, elem.first) == 1)
							{
								cout << elem.first << endl;
								text = elem.second;
								line = esm.getCollText(i).erase(esm.getCollTextPos(i), esm.getCollTextSize(i));
								cout << line << endl;
								line.insert(esm.getCollTextPos(i), "\"" + elem.second + "\"");
								cout << line << endl;
								counter++;
								found = 1;
								break;
							}
						}
						if(found == 1)
						{
							sec_text += line + "\r\n";
						}
						else
						{
							sec_text += esm.getCollText(i) + "\r\n";
						}
					}

				}
				sec_text.erase(sec_text.size() - 2);
				// subrecord text
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
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "DIAL in BNAM records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertCellInBnam()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string sec_text;
	string line;
	string text;
	bool found;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "INFO")
		{
			esm.setPriSubRec("INAM");
			esm.setSecSubRec("BNAM");
			esm.setColl(esmtools::CELL);
			sec_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					if(esm.getCollTextStatus(i) == 0)
					{
						sec_text += esm.getCollText(i) + "\r\n";
					}
					else
					{
						text = esm.getCollText(i).substr(esm.getCollTextPos(i), esm.getCollTextSize(i));
						if(text.find("\"") != string::npos)
						{
							text = text.substr(1);
							text = text.erase(text.find("\""));
						}
						text = "CELL" + sep[0] + text;
						cout << text << endl;
						found = 0;
						for(auto &elem : dict.getDict())
						{
							if(caseInsensitiveStringCmp(text, elem.first) == 1)
							{
								cout << elem.first << endl;
								text = elem.second;
								line = esm.getCollText(i).erase(esm.getCollTextPos(i), esm.getCollTextSize(i));
								cout << line << endl;
								line.insert(esm.getCollTextPos(i), "\"" + elem.second + "\"");
								cout << line << endl;
								counter++;
								found = 1;
								break;
							}
						}
						if(found == 1)
						{
							sec_text += line + "\r\n";
						}
						else
						{
							sec_text += esm.getCollText(i) + "\r\n";
						}
					}

				}
				sec_text.erase(sec_text.size() - 2);
				// subrecord text
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
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "CELL in BNAM records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertScpt()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string sec_text;
	string line;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "SCPT")
		{
			esm.setPriSubRec("SCHD");
			esm.setSecSubRec("SCTX");
			esm.setColl(esmtools::SCPT);
			sec_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					if(esm.getCollTextStatus(i) == false)
					{
						sec_text += esm.getCollText(i) + "\r\n";
					}
					else
					{
						auto search = dict.getDict().find(esm.getRecId() + sep[0] + esm.getCollText(i));
						if(search == dict.getDict().end())
						{
							sec_text += esm.getCollText(i) + "\r\n";
						}
						else
						{
							line = search->second;
							line = line.substr(line.find(sep[0]) + sep[0].size());
							sec_text += line + "\r\n";
							counter++;
						}
					}
				}
				sec_text.erase(sec_text.size() - 2);
				// subrecord text
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
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "SCPT script lines converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertDialInScpt()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string sec_text;
	string line;
	string text;
	bool found;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "SCPT")
		{
			esm.setPriSubRec("SCHD");
			esm.setSecSubRec("SCTX");
			esm.setColl(esmtools::DIAL);
			sec_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					if(esm.getCollTextStatus(i) == 0)
					{
						sec_text += esm.getCollText(i) + "\r\n";
					}
					else
					{
						text = esm.getCollText(i).substr(esm.getCollTextPos(i), esm.getCollTextSize(i));
						if(text.find("\"") == 0 && text.rfind("\"") == text.size() - 1)
						{
							text = text.substr(1, text.size() - 2);
						}
						text = "DIAL" + sep[0] + text;
						cout << text << endl;
						found = 0;
						for(auto &elem : dict.getDict())
						{
							if(caseInsensitiveStringCmp(text, elem.first) == 1)
							{
								cout << elem.first << endl;
								text = elem.second;
								line = esm.getCollText(i).erase(esm.getCollTextPos(i), esm.getCollTextSize(i));
								cout << line << endl;
								line.insert(esm.getCollTextPos(i), "\"" + elem.second + "\"");
								cout << line << endl;
								counter++;
								found = 1;
								break;
							}
						}
						if(found == 1)
						{
							sec_text += line + "\r\n";
						}
						else
						{
							sec_text += esm.getCollText(i) + "\r\n";
						}
					}

				}
				sec_text.erase(sec_text.size() - 2);
				// subrecord text
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
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "DIAL in SCPT records converted: " << counter << endl;
}

//----------------------------------------------------------
void converter::convertCellInScpt()
{
	string rec_content;
	string esm_content;
	size_t sec_size = 0;
	size_t rec_size = 0;
	string sec_text;
	string line;
	string text;
	bool found;
	int counter = 0;
	esm.resetRec();
	while(esm.loopCheck())
	{
		esm.setNextRec();
		esm.setRecContent();
		rec_content = esm.getRecContent();
		if(esm.getRecId() == "SCPT")
		{
			esm.setPriSubRec("SCHD");
			esm.setSecSubRec("SCTX");
			esm.setColl(esmtools::CELL);
			sec_text.erase();
			if(esm.getCollSize() > 0)
			{
				for(size_t i = 0; i < esm.getCollSize(); i++)
				{
					if(esm.getCollTextStatus(i) == 0)
					{
						sec_text += esm.getCollText(i) + "\r\n";
					}
					else
					{
						text = esm.getCollText(i).substr(esm.getCollTextPos(i), esm.getCollTextSize(i));
						if(text.find("\"") != string::npos)
						{
							text = text.substr(1);
							text = text.erase(text.find("\""));
						}
						text = "CELL" + sep[0] + text;
						cout << text << endl;
						found = 0;
						for(auto &elem : dict.getDict())
						{
							if(caseInsensitiveStringCmp(text, elem.first) == 1)
							{
								cout << elem.first << endl;
								text = elem.second;
								line = esm.getCollText(i).erase(esm.getCollTextPos(i), esm.getCollTextSize(i));
								cout << line << endl;
								line.insert(esm.getCollTextPos(i), "\"" + elem.second + "\"");
								cout << line << endl;
								counter++;
								found = 1;
								break;
							}
						}
						if(found == 1)
						{
							sec_text += line + "\r\n";
						}
						else
						{
							sec_text += esm.getCollText(i) + "\r\n";
						}
					}

				}
				sec_text.erase(sec_text.size() - 2);
				// subrecord text
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
			}
		}
		esm_content.append(rec_content);
	}
	esm.setEsmContent(esm_content);
	cerr << "CELL in SCPT records converted: " << counter << endl;
}
