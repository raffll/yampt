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

//----------------------------------------------------------
void EsmConverter::resetCounters()
{
	counter = 0;
}

//----------------------------------------------------------
void EsmConverter::setNewFriendly(RecType type)
{
	found_n = false;
	equal_n = false;
	result = "Not converted";
	new_friendly = "N\\A";

	auto search_n = merger.getDict()[type].find(unique_n);
	if(search_n != merger.getDict()[type].end())
	{
		found_n = true;

		if(esm.getFriendlyId() == "SCVR")
		{
			new_friendly = esm.getFriendly().substr(0, 5) + search_n->second;
		}
		else
		{
			new_friendly = search_n->second;
		}

		if(esm.getFriendly() == new_friendly)
		{
			equal_n = true;
			result = "Skipped, identical as in dictionary";
		}
	}
}

//----------------------------------------------------------
void EsmConverter::setSafeConditions(RecType type)
{
	found_f = false;
	equal_f = false;

	if(Config::getSafeConvert() == true)
	{
		auto search_f = merger.getDict()[type].find(unique_f);
		if(search_f != merger.getDict()[type].end())
		{
			found_f = true;

			if(esm.getFriendly() == search_f->second)
			{
				equal_f = true;
			}
			else
			{
				equal_f = false;
				result = "Not converted, text is different from original dictionary";
			}
		}
	}
}

//----------------------------------------------------------
void EsmConverter::makeDetailedLog(string id, bool safe)
{
	log_detailed += "File:                        | " + esm.getName() + "\r\n" +
			"Record:                      | " + id + " " + esm.getUnique() + "\r\n" +
			"Found in dictionary / equal  | " + to_string(found_n) + "/" + to_string(equal_n) + "\r\n";
	if(safe == true)
	{
		log_detailed += "Found in original / equal  | " + to_string(found_f) + "/" + to_string(equal_f) + "\r\n";
	}
	log_detailed += "Result                       | " + result + "\r\n" +
			line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::printLog(string id)
{
	cout << "    --> " << id << " records converted: " << to_string(counter) << endl;
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent(string new_text)
{
	size_t rec_size;
	string rec_content = esm.getRecContent();

	if(esm.getFriendlyStatus() == true)
	{
		rec_content.erase(esm.getFriendlyPos() + 8, esm.getFriendlySize());
		rec_content.insert(esm.getFriendlyPos() + 8, new_text);
		rec_content.erase(esm.getFriendlyPos() + 4, 4);
		rec_content.insert(esm.getFriendlyPos() + 4, convertIntToByteArray(new_text.size()));
		rec_size = rec_content.size() - 16;
		rec_content.erase(4, 4);
		rec_content.insert(4, convertIntToByteArray(rec_size));
		esm.setRecContent(rec_content);

		result = "Converted";
		counter++;
	}
}

//----------------------------------------------------------
void EsmConverter::convertCELL()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "CELL")
		{
			esm.setUnique("NAME");
			esm.setFriendly("NAME");

			if(esm.getUniqueStatus() == true)
			{
				unique_n = "CELL" + sep[0] + esm.getUnique();
				setNewFriendly(RecType::CELL);

				if(found_n == true && equal_n == false)
				{
					convertRecordContent(new_friendly + '\0');
				}
				makeDetailedLog("CELL");
			}
		}
	}
	printLog("CELL");
}

//----------------------------------------------------------
void EsmConverter::convertPGRD()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "PGRD")
		{
			esm.setUnique("NAME");
			esm.setFriendly("NAME");

			unique_n = "CELL" + sep[0] + esm.getUnique();
			setNewFriendly(RecType::CELL);

			if(found_n == true && equal_n == false)
			{
				convertRecordContent(new_friendly + '\0');
			}
			makeDetailedLog("PGRD");
		}
	}
	printLog("PGRD");
}

//----------------------------------------------------------
void EsmConverter::convertANAM()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setUnique("ANAM");
			esm.setFriendly("ANAM");

			if(esm.getUniqueStatus() == true)
			{
				unique_n = "CELL" + sep[0] + esm.getUnique();
				setNewFriendly(RecType::CELL);

				if(found_n == true && equal_n == false)
				{
					convertRecordContent(new_friendly + '\0');
				}
				makeDetailedLog("ANAM");
			}
		}
	}
	printLog("ANAM");
}

//----------------------------------------------------------
void EsmConverter::convertSCVR()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setUnique("INAM");
			esm.setFriendly("SCVR");

			while(esm.getFriendlyStatus() == true)
			{
				if(esm.getFriendly().substr(1, 1) == "B")
				{
					unique_n = "CELL" + sep[0] + esm.getFriendly().substr(5);
					setNewFriendly(RecType::CELL);

					if(found_n == true && equal_n == false)
					{
						convertRecordContent(new_friendly);
					}
					makeDetailedLog("SCVR");
				}
				esm.setFriendly("SCVR", NEXT);
			}
		}
	}
	printLog("SCVR");
}

//----------------------------------------------------------
void EsmConverter::convertDNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "CELL" ||
		   esm.getRecId() == "NPC_")
		{
			esm.setUnique("NAME");
			esm.setFriendly("DNAM");

			while(esm.getFriendlyStatus() == true)
			{
				unique_n = "CELL" + sep[0] + esm.getFriendly();
				setNewFriendly(RecType::CELL);

				if(found_n == true && equal_n == false)
				{
					convertRecordContent(new_friendly + '\0');

				}
				makeDetailedLog("DNAM");
				esm.setFriendly("DNAM", NEXT);
			}
		}
	}
	printLog("DNAM");
}

//----------------------------------------------------------
void EsmConverter::convertCNDT()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "NPC_")
		{
			esm.setUnique("NAME");
			esm.setFriendly("CNDT");

			while(esm.getFriendlyStatus() == true)
			{
				unique_n = "CELL" + sep[0] + esm.getFriendly();
				setNewFriendly(RecType::CELL);

				if(found_n == true && equal_n == false)
				{
					convertRecordContent(new_friendly + '\0');
				}
				makeDetailedLog("CNDT");
				esm.setFriendly("CNDT", NEXT);
			}
		}
	}
	printLog("CNDT");
}

//----------------------------------------------------------
void EsmConverter::convertGMST()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "GMST")
		{
			esm.setUnique("NAME");
			esm.setFriendly("STRV");

			if(esm.getUnique().substr(0, 1) == "s")
			{
				unique_n = "GMST" + sep[0] + esm.getUnique();
				unique_f = "GMST" + ext + sep[0] + esm.getUnique();

				setNewFriendly(RecType::GMST);
				setSafeConditions(RecType::GMST);

				if(Config::getSafeConvert() == false &&
				   found_n == true && equal_n == false)
				{
					convertRecordContent(new_friendly + '\0');
				}
				else if(found_n == true && equal_n == false &&
					found_f == true && equal_f == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
				makeDetailedLog("GMST", true);
			}
		}
	}
	printLog("GMST");
}

//----------------------------------------------------------
void EsmConverter::convertFNAM()
{
	resetCounters();

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
			esm.setUnique("NAME");
			esm.setFriendly("FNAM");

			unique_n = "FNAM" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();
			unique_f = "FNAM" + ext + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();

			setNewFriendly(RecType::FNAM);
			setSafeConditions(RecType::FNAM);

			if(Config::getSafeConvert() == false &&
			   found_n == true && equal_n == false)
			{
				convertRecordContent(new_friendly + '\0');
			}
			else if(found_n == true && equal_n == false &&
				found_f == true && equal_f == true)
			{
				convertRecordContent(new_friendly + '\0');
			}
			makeDetailedLog("FNAM", true);
		}
	}
	printLog("FNAM");
}

//----------------------------------------------------------
void EsmConverter::convertDESC()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BSGN" ||
		   esm.getRecId() == "CLAS" ||
		   esm.getRecId() == "RACE")
		{
			esm.setUnique("NAME");
			esm.setFriendly("DESC");

			unique_n = "DESC" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();
			unique_f = "DESC" + ext + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();

			setNewFriendly(RecType::DESC);
			setSafeConditions(RecType::DESC);

			if(Config::getSafeConvert() == false &&
			   found_n == true && equal_n == false)
			{
				convertRecordContent(new_friendly + '\0');
			}
			else if(found_n == true && equal_n == false &&
				found_f == true && equal_f == true)
			{
				convertRecordContent(new_friendly + '\0');
			}
			makeDetailedLog("DESC", true);
		}
	}
	printLog("DESC");
}

//----------------------------------------------------------
void EsmConverter::convertTEXT()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "BOOK")
		{
			esm.setUnique("NAME");
			esm.setFriendly("TEXT");

			unique_n = "TEXT" + sep[0] + esm.getUnique();
			unique_f = "TEXT" + ext + sep[0] + esm.getUnique();

			setNewFriendly(RecType::TEXT);
			setSafeConditions(RecType::TEXT);

			if(Config::getSafeConvert() == false &&
			   found_n == true && equal_n == false)
			{
				convertRecordContent(new_friendly + '\0');
			}
			else if(found_n == true && equal_n == false &&
				found_f == true && equal_f == true)
			{
				convertRecordContent(new_friendly + '\0');
			}
			makeDetailedLog("TEXT", true);
		}
	}
	printLog("TEXT");
}

//----------------------------------------------------------
void EsmConverter::convertRNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "FACT")
		{
			esm.setUnique("NAME");
			esm.setFriendly("RNAM");

			while(esm.getFriendlyStatus() == true)
			{
				unique_n = "RNAM" + sep[0] + esm.getUnique() + sep[0] + to_string(esm.getFriendlyCounter());
				unique_f = "RNAM" + ext + sep[0] + esm.getUnique() + sep[0] + to_string(esm.getFriendlyCounter());

				setNewFriendly(RecType::RNAM);
				setSafeConditions(RecType::RNAM);

				new_friendly.resize(32);

				if(Config::getSafeConvert() == false &&
				   found_n == true && equal_n == false)
				{
					convertRecordContent(new_friendly);
				}
				else if(found_n == true && equal_n == false &&
					found_f == true && equal_f == true)
				{
					convertRecordContent(new_friendly);
				}
				makeDetailedLog("RNAM", true);
				esm.setFriendly("RNAM", NEXT);
			}
		}
	}
	printLog("RNAM");
}

//----------------------------------------------------------
void EsmConverter::convertINDX()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "SKIL" ||
		   esm.getRecId() == "MGEF")
		{
			esm.setUnique("INDX");
			esm.setFriendly("DESC");

			unique_n = "INDX" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();
			unique_f = "INDX" + ext + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();

			setNewFriendly(RecType::INDX);
			setSafeConditions(RecType::INDX);

			if(Config::getSafeConvert() == false &&
			   found_n == true && equal_n == false)
			{
				convertRecordContent(new_friendly + '\0');
			}
			else if(found_n == true && equal_n == false &&
				found_f == true && equal_f == true)
			{
				convertRecordContent(new_friendly + '\0');
			}
			makeDetailedLog("INDX", true);
		}
	}
	printLog("INDX");
}

//----------------------------------------------------------
void EsmConverter::convertDIAL()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setUnique("NAME");
			esm.setFriendly("NAME");

			unique_n = "DIAL" + sep[0] + esm.getFriendly();
			setNewFriendly(RecType::DIAL);

			if(found_n == true && equal_n == false)
			{
				convertRecordContent(new_friendly + '\0');
			}
			makeDetailedLog("DIAL");
		}
	}
	printLog("DIAL");
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
	resetCounters();
	string dial;

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setUnique("DATA");
			esm.setFriendly("NAME");

			dial = esm.getUnique() + sep[0] + esm.getFriendly();
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setUnique("INAM");
			esm.setFriendly("NAME");

			unique_n = "INFO" + sep[0] + dial + sep[0] + esm.getUnique();
			unique_f = "INFO" + ext + sep[0] + dial + sep[0] + esm.getUnique();

			setNewFriendly(RecType::INFO);
			setSafeConditions(RecType::INFO);

			if(Config::getSafeConvert() == false &&
			   found_n == true && equal_n == false)
			{
				convertRecordContent(new_friendly + '\0');
			}
			else if(found_n == true && equal_n == false &&
				found_f == true && equal_f == true)
			{
				convertRecordContent(new_friendly + '\0');
			}
			else if(Config::getAddDialToInfo() == true &&
				dial.substr(0, 1) != "V")
			{
				addDIALtoINFO();
				convertRecordContent(new_friendly + '\0');
			}
			makeDetailedLog("INFO");
		}
	}
	printLog("INFO");
}

//----------------------------------------------------------
void EsmConverter::addDIALtoINFO()
{
	string new_friendly_lc;
	size_t pos;
	string unique;

	for(auto &elem : merger.getDict()[RecType::DIAL])
	{
		unique = elem.first.substr(5);
		if(unique != elem.second)
		{
			new_friendly_lc = new_friendly;

			transform(new_friendly_lc.begin(), new_friendly_lc.end(),
				  new_friendly_lc.begin(), ::tolower);

			transform(unique.begin(), unique.end(),
				  unique.begin(), ::tolower);

			pos = new_friendly_lc.find(unique);
			if(pos != string::npos)
			{
				new_friendly.insert(new_friendly.size(), " [" + elem.second + "]");
			}
		}
	}
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "INFO")
		{
			esm.setFriendly("BNAM");

			if(esm.getFriendlyStatus() == true)
			{
				convertScript(RecType::BNAM, "BNAM");
				if(esm.getFriendly() != new_friendly)
				{
					convertRecordContent(new_friendly);
				}
			}
		}
	}
	printLog("BNAM");
}

//----------------------------------------------------------
void EsmConverter::convertSCPT()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "SCPT")
		{
			esm.setFriendly("SCTX");

			if(esm.getFriendlyStatus() == true)
			{
				convertScript(RecType::SCTX, "SCTX");
				if(esm.getFriendly() != new_friendly)
				{
					convertRecordContent(new_friendly);
				}
			}
		}
	}
	printLog("SCTX");
}

//----------------------------------------------------------
void EsmConverter::convertScript(RecType type, string id)
{
	if(status == true && esm.getFriendlyStatus() == true)
	{
		bool found;
		string line;
		string line_lc;
		size_t pos;
		string text;

		new_friendly.erase();
		istringstream ss(esm.getFriendly());

		while(getline(ss, line))
		{
			found = false;
			eraseCarriageReturnChar(line);
			line_lc = line;
			transform(line_lc.begin(), line_lc.end(),
				  line_lc.begin(), ::tolower);

			for(auto const &elem : key_message)
			{
				if(found == false)
				{
					pos = line_lc.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						auto search = merger.getDict()[type].find(id + sep[0] + line);
						if(search != merger.getDict()[type].end())
						{
							if(line != search->second)
							{
								line = search->second;
								line = line.substr(line.find(sep[0]) + 1);
								found = true;
							}
						}
					}
				}
			}
			for(auto const &elem : key_dial)
			{
				if(found == false)
				{
					pos = line_lc.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						extractText(line, text, pos);
						auto search = merger.getDict()[RecType::DIAL].find("DIAL" + sep[0] + text);
						if(search != merger.getDict()[RecType::DIAL].end())
						{
							if(text != search->second)
							{
								line.erase(pos, text.size() + 2);
								line.insert(pos, "\"" + search->second + "\"");
								found = true;
							}
						}
						if(found == false)
						{
							for(auto &elem : merger.getDict()[RecType::DIAL])
							{
								if(caseInsensitiveStringCmp("DIAL" + sep[0] + text, elem.first) == true)
								{
									line.erase(pos, text.size() + 2);
									line.insert(pos, "\"" + elem.second + "\"");
									found = true;
									break;
								}
							}
						}
					}
				}
			}
			for(auto const &elem : key_cell)
			{
				if(found == false)
				{
					pos = line_lc.find(elem);
					if(pos != string::npos && line.rfind(";", pos) == string::npos)
					{
						extractText(line, text, pos);
						auto search = merger.getDict()[RecType::CELL].find("CELL" + sep[0] + text);
						if(search != merger.getDict()[RecType::CELL].end())
						{
							if(text != search->second)
							{
								line.erase(pos, text.size() + 2);
								line.insert(pos, "\"" + search->second + "\"");
								found = true;
							}
						}
						if(found == false)
						{
							for(auto &elem : merger.getDict()[RecType::CELL])
							{
								if(caseInsensitiveStringCmp("CELL" + sep[0] + text, elem.first) == true)
								{
									line.erase(pos, text.size() + 2);
									line.insert(pos, "\"" + elem.second + "\"");
									found = true;
									break;
								}
							}
						}
					}
				}
			}
			new_friendly += line + "\r\n";
		}

		size_t last_nl_pos = esm.getFriendly().rfind("\r\n");
		if(last_nl_pos != esm.getFriendly().size() - 2 || last_nl_pos == string::npos)
		{
			new_friendly.resize(new_friendly.size() - 2);
		}
	}
}

//----------------------------------------------------------
void EsmConverter::extractText(const string &line, string &text, size_t &pos)
{
	if(line.find("\"", pos) != string::npos)
	{
		regex re("\"(.*?)\"");
		smatch found;
		sregex_iterator next(line.begin(), line.end(), re);
		sregex_iterator end;
		while(next != end)
		{
			found = *next;
			text = found[1].str();
			pos = found.position(0);
			next++;
		}
	}
	else
	{
		size_t last_ws_pos;
		pos = line.find(" ", pos);
		pos = line.find_first_not_of(" ", pos);
		text = line.substr(pos);
		last_ws_pos = text.find_last_not_of(" \t");
		if(last_ws_pos != string::npos)
		{
			text.erase(last_ws_pos + 1);
		}
	}
}
