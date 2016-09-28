#include "EsmConverter.hpp"

using namespace std;

//----------------------------------------------------------
EsmConverter::EsmConverter(string path, DictMerger &merger, bool safe, bool add_dial)
{
	this->merger = &merger;
	this->safe = safe;
	this->add_dial = add_dial;

	esm.readFile(path);

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
		printLog("", true);
		convertCELL();
		convertPGRD();
		convertANAM();
		convertSCVR();
		convertDNAM();
		convertCNDT();
		if(safe == false)
		{
			convertGMST();
			convertFNAM();
			convertDESC();
			convertTEXT();
			convertRNAM();
			convertINDX();
		}
		convertDIAL();
		convertINFO();
		convertBNAM();
		convertSCPT();
		cout << endl;
	}
}

//----------------------------------------------------------
void EsmConverter::makeLog(string id)
{
	log += "<!-- " + id + " record " + *result_ptr + " in " + esm.getName() + " -->" + "\r\n" +
	       yampt::sep[1] + unique_key + yampt::sep[2] + esm.getFriendly() + yampt::sep[3] + "\r\n" +
	       "<!-- >>> -->\r\n" +
	       yampt::sep[1] + unique_key + yampt::sep[2] + new_friendly + yampt::sep[3] + "\r\n\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLogScriptBefore()
{
	log += "<!-- " + esm.getFriendlyId() + " " + esm.getUnique() + " script line in " + esm.getName() + " -->" + "\r\n" +
	       s_line + "\r\n" +
	       "<!-- >>> -->\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLogScriptAfter()
{
	log += s_line + "\r\n\r\n";
}

//----------------------------------------------------------
void EsmConverter::printLog(string id, bool header)
{
	if(header == true)
	{
		cout << endl;
		cout << "          Converted / Not converted / Skipped /   All" << endl;
		cout << "    -------------------------------------------------" << endl;
	}
	else
	{
		cout << "    " << id << " "
		     << setw(10) << to_string(counter_converted) << " / "
		     << setw(13) << to_string(counter_notfound) << " / "
		     << setw(7) << to_string(counter_skipped) << " / "
		     << setw(5) << to_string(counter_all) << endl;
		if(id == "INFO" && add_dial == true)
		{
			cout << "    " << " + DIAL" << " "
			     << setw(7) << to_string(counter_add) << " / "
			     << setw(13) << "-" << " / "
			     << setw(7) << "-" << " / "
			     << setw(5) << "-" << endl;
		}
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
	counter_converted = 0;
	counter_notfound = 0;
	counter_skipped = 0;
	counter_all = 0;
	counter_add = 0;
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
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendly(yampt::r_type type)
{
	counter_all++;

	auto search = merger->getDict()[type].find(unique_key);
	if(search != merger->getDict()[type].end())
	{
		if(esm.getFriendlyId() == "SCVR")
		{
			new_friendly = esm.getFriendly().substr(0, 5) + search->second;
		}
		else
		{
			new_friendly = search->second;
		}

		if(esm.getFriendly() != new_friendly)
		{
			convert = true;
			result_ptr = &yampt::result[1];
			counter_converted++;
		}
		else
		{
			convert = false;
			result_ptr = &yampt::result[2];
			counter_skipped++;
		}
	}
	else
	{
		convert = false;
		result_ptr = &yampt::result[0];
		counter_notfound++;

		new_friendly = "N\\A";
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendlyINFO(yampt::r_type type)
{
	counter_all++;

	auto search = merger->getDict()[type].find(unique_key);
	if(safe == false && search != merger->getDict()[type].end())
	{
		new_friendly = search->second;

		if(esm.getFriendly() != new_friendly)
		{
			convert = true;
			result_ptr = &yampt::result[1];
			counter_converted++;
		}
		else
		{
			convert = false;
			result_ptr = &yampt::result[2];
			counter_skipped++;
		}
	}
	else if(add_dial == true && esm.getRecId() == "INFO" && current_dialog.substr(0, 1) != "V")
	{
		addDIALtoINFO();

		if(esm.getFriendly() != new_friendly)
		{
			convert = true;
			result_ptr = &yampt::result[3];
			counter_add++;
		}
		else
		{
			convert = false;
			result_ptr = &yampt::result[2];
			counter_skipped++;
		}
	}
	else
	{
		convert = false;
		result_ptr = &yampt::result[0];
		counter_notfound++;

		new_friendly = "N\\A";
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendlyScript(string id, yampt::r_type type)
{
	counter_all++;
	new_friendly.erase();
	istringstream ss(esm.getFriendly());

	while(getline(ss, s_line))
	{
		s_found = false;
		eraseCarriageReturnChar(s_line);
		s_line_lc = s_line;
		transform(s_line_lc.begin(), s_line_lc.end(),
			  s_line_lc.begin(), ::tolower);

		for(auto const &elem : yampt::key_message)
		{
			if(s_found == false)
			{
				s_pos = s_line_lc.find(elem);
				convertLine(id, type);
			}
		}

		for(auto const &elem : yampt::key_dial)
		{
			if(s_found == false)
			{
				s_pos = s_line_lc.find(elem);
				convertText("DIAL", yampt::r_type::DIAL);
			}
		}

		for(auto const &elem : yampt::key_cell)
		{
			if(s_found == false)
			{
				s_pos = s_line_lc.find(elem);
				convertText("CELL", yampt::r_type::CELL);
			}
		}

		new_friendly += s_line + "\r\n";
	}

	size_t last_nl_pos = esm.getFriendly().rfind("\r\n");
	if(last_nl_pos != esm.getFriendly().size() - 2 || last_nl_pos == string::npos)
	{
		new_friendly.resize(new_friendly.size() - 2);
	}

	if(esm.getFriendly() != new_friendly)
	{
		convert = true;
		result_ptr = &yampt::result[1];
		counter_converted++;
	}
	else
	{
		convert = false;
		result_ptr = &yampt::result[0];
		counter_notfound++;

		new_friendly = "N\\A";
	}
}

//----------------------------------------------------------
void EsmConverter::convertLine(string id, yampt::r_type type)
{
	if(s_pos != string::npos && s_line.rfind(";", s_pos) == string::npos)
	{
		makeLogScriptBefore();
		auto search = merger->getDict()[type].find(id + yampt::sep[0] + s_line);
		if(search != merger->getDict()[type].end())
		{
			if(s_line != search->second)
			{
				s_line = search->second;
				s_line = s_line.substr(s_line.find(yampt::sep[0]) + 1);
				s_found = true;
			}
		}
		makeLogScriptAfter();
	}
}

//----------------------------------------------------------
void EsmConverter::convertText(string id, yampt::r_type type)
{
	if(s_pos != string::npos && s_line.rfind(";", s_pos) == string::npos)
	{
		makeLogScriptBefore();
		extractText();
		auto search = merger->getDict()[type].find(id + yampt::sep[0] + s_text);
		if(search != merger->getDict()[type].end())
		{
			if(s_text != search->second)
			{
				s_line.erase(s_pos, s_text.size() + 2);
				s_line.insert(s_pos, "\"" + search->second + "\"");
				s_found = true;
			}
		}
		else
		{
			for(auto &elem : merger->getDict()[type])
			{
				if(caseInsensitiveStringCmp(id + yampt::sep[0] + s_text, elem.first) == true)
				{
					s_line.erase(s_pos, s_text.size() + 2);
					s_line.insert(s_pos, "\"" + elem.second + "\"");
					s_found = true;
					break;
				}
			}
		}
		makeLogScriptAfter();
	}
}

//----------------------------------------------------------
void EsmConverter::extractText()
{
	if(s_line.find("\"", s_pos) != string::npos)
	{
		regex re("\"(.*?)\"");
		smatch found;
		sregex_iterator next(s_line.begin(), s_line.end(), re);
		sregex_iterator end;
		while(next != end)
		{
			found = *next;
			s_text = found[1].str();
			s_pos = found.position(0);
			next++;
		}
	}
	else
	{
		size_t last_ws_pos;
		s_pos = s_line.find(" ", s_pos);
		s_pos = s_line.find_first_not_of(" ", s_pos);
		s_text = s_line.substr(s_pos);
		last_ws_pos = s_text.find_last_not_of(" \t");
		if(last_ws_pos != string::npos)
		{
			s_text.erase(last_ws_pos + 1);
		}
	}
}

//----------------------------------------------------------
void EsmConverter::addDIALtoINFO()
{
	string key;
	string new_friendly_lc;
	size_t pos;

	new_friendly = esm.getFriendly();

	for(auto &elem : merger->getDict()[yampt::r_type::DIAL])
	{
		key = elem.first.substr(5);
		if(key != elem.second)
		{
			new_friendly_lc = new_friendly;

			transform(new_friendly_lc.begin(), new_friendly_lc.end(),
				  new_friendly_lc.begin(), ::tolower);

			transform(key.begin(), key.end(),
				  key.begin(), ::tolower);

			pos = new_friendly_lc.find(key);
			if(pos != string::npos)
			{
				new_friendly.insert(new_friendly.size(), " [" + elem.second + "]");
			}
		}
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
				unique_key = "CELL" + yampt::sep[0] + esm.getUnique();
				setNewFriendly(yampt::r_type::CELL);
				makeLog("CELL");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
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

			unique_key = "CELL" + yampt::sep[0] + esm.getUnique();
			setNewFriendly(yampt::r_type::CELL);
			makeLog("PGRD");
			if(convert == true)
			{
				convertRecordContent(new_friendly + '\0');
			}
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
				unique_key = "CELL" + yampt::sep[0] + esm.getUnique();
				setNewFriendly(yampt::r_type::CELL);
				makeLog("ANAM");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
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
					unique_key = "CELL" + yampt::sep[0] + esm.getFriendly().substr(5);
					setNewFriendly(yampt::r_type::CELL);
					makeLog("SCVR");
					if(convert == true)
					{
						convertRecordContent(new_friendly);
					}
				}
				esm.setFriendly("SCVR", true);
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
				unique_key = "CELL" + yampt::sep[0] + esm.getFriendly();
				setNewFriendly(yampt::r_type::CELL);
				makeLog("DNAM");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');

				}
				esm.setFriendly("DNAM", true);
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
				unique_key = "CELL" + yampt::sep[0] + esm.getFriendly();
				setNewFriendly(yampt::r_type::CELL);
				makeLog("CNDT");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
				esm.setFriendly("CNDT", true);
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

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() && esm.getUnique().substr(0, 1) == "s")
			{
				unique_key = "GMST" + yampt::sep[0] + esm.getUnique();
				setNewFriendly(yampt::r_type::GMST);
				makeLog("GMST");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
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

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() == true && esm.getUnique() != "player")
			{
				unique_key = "FNAM" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique();
				setNewFriendly(yampt::r_type::FNAM);
				makeLog("FNAM");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
			}
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

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() == true)
			{
				unique_key = "DESC" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique();
				setNewFriendly(yampt::r_type::DESC);
				makeLog("DESC");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
			}
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

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() == true)
			{
				unique_key = "TEXT" + yampt::sep[0] + esm.getUnique();
				setNewFriendly(yampt::r_type::TEXT);
				makeLog("TEXT");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
			}
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

			if(esm.getUniqueStatus() == true)
			{
				while(esm.getFriendlyStatus() == true)
				{
					unique_key = "RNAM" + yampt::sep[0] + esm.getUnique() + yampt::sep[0] + to_string(esm.getFriendlyCounter());
					setNewFriendly(yampt::r_type::RNAM);
					makeLog("RNAM");
					if(convert == true)
					{
						new_friendly.resize(32);
						convertRecordContent(new_friendly);
					}
					esm.setFriendly("RNAM", true);
				}
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

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() == true)
			{
				unique_key = "INDX" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique();
				setNewFriendly(yampt::r_type::INDX);
				makeLog("INDX");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
			}
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
			esm.setUnique("DATA");
			esm.setFriendly("NAME");

			if(esm.getUnique() == "T")
			{
				unique_key = "DIAL" + yampt::sep[0] + esm.getFriendly();
				setNewFriendly(yampt::r_type::DIAL);
				makeLog("DIAL");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
			}
		}
	}
	printLog("DIAL");
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
	resetCounters();

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "DIAL")
		{
			esm.setUnique("DATA");
			esm.setFriendly("NAME");

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() == true)
			{
				current_dialog = esm.getUnique() + yampt::sep[0] + esm.getFriendly();
			}
			else
			{
				current_dialog = "<NotFound>";
			}
		}
		if(esm.getRecId() == "INFO")
		{
			esm.setUnique("INAM");
			esm.setFriendly("NAME");

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() == true)
			{
				unique_key = "INFO" + yampt::sep[0] + current_dialog + yampt::sep[0] + esm.getUnique();
				setNewFriendlyINFO(yampt::r_type::INFO);
				makeLog("INFO");
				if(convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
			}
		}
	}
	printLog("INFO");
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
			esm.setUnique("INAM");
			esm.setFriendly("BNAM");

			if(esm.getFriendlyStatus() == true)
			{
				setNewFriendlyScript("BNAM", yampt::r_type::BNAM);
				if(convert == true)
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
			esm.setUnique("SCHD");
			esm.setFriendly("SCTX");

			if(esm.getFriendlyStatus() == true)
			{
				setNewFriendlyScript("SCTX", yampt::r_type::SCTX);
				if(convert == true)
				{
					convertRecordContent(new_friendly);
				}
			}
		}
	}
	printLog("SCTX");
}
