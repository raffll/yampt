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
		convertGMDT();
		cout << endl;
	}
}

//----------------------------------------------------------
void EsmConverter::makeLog(string key)
{
	log += *result_ptr + " '" + key + "' in '" + esm.getName() + "'\r\n";
	if(result_ptr != &yampt::result[2])
        {
                log += esm.getFriendly() + " -->" + "\r\n" +
		       new_friendly + "\r\n";
        }
        log += "---\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLogScript(string key)
{
	log += *result_ptr + " script line '" + key + "' in '" + esm.getName() + "'\r\n" +
	       line + " -->" + "\r\n" +
	       line_new + "\r\n" +
	       "---" + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::printLog(string id, bool header)
{
	if(header == true)
	{
		cout << endl
		     << "          CONVERTED / UNCHANGED / SKIPPED /    ALL" << endl
		     << "    ----------------------------------------------" << endl;
	}
	else
	{
		if(id == "INFO" && add_dial == true)
		{
			if(counter_add > 0)
			{
				cout << endl;
			}

			cout << "    " << id << " "
			     << setw(10) << to_string(counter_converted) << " / "
			     << setw(9) << to_string(counter_unchanged) << " / "
			     << setw(7) << to_string(counter_skipped) << " / "
			     << setw(6) << to_string(counter_all) << endl
			     << "    " << " + DIAL" << " "
			     << setw(7) << to_string(counter_add) << " / "
			     << setw(9) << "-" << " / "
			     << setw(7) << "-" << " / "
			     << setw(6) << "-" << endl;
		}
		else if(id == "SCTX" || id == "BNAM")
		{
			cout << "    " << id << " "
			     << setw(10) << to_string(counter_converted) << " / "
			     << setw(9) << to_string(counter_unchanged) << " / "
			     << setw(7) << "-" << " / "
			     << setw(6) << to_string(counter_all) << endl;
		}
		else
		{
			cout << "    " << id << " "
			     << setw(10) << to_string(counter_converted) << " / "
			     << setw(9) << to_string(counter_unchanged) << " / "
			     << setw(7) << to_string(counter_skipped) << " / "
			     << setw(6) << to_string(counter_all) << endl;
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
	counter_unchanged = 0;
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
		counter_unchanged++;

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
			if(counter_add == 1)
			{
				cout << "    In progress...";
			}
			counter_add++;
			if(counter_add % 200 == 0)
			{
				cout << "." << flush;
			}
		}
		else
		{
			convert = false;
			result_ptr = &yampt::result[0];
			counter_unchanged++;

			new_friendly = "N\\A";
		}
	}
	else
	{
		convert = false;
		result_ptr = &yampt::result[0];
		counter_unchanged++;

		new_friendly = "N\\A";
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendlyScript(string id, yampt::r_type type)
{
	counter_all++;
	new_friendly.erase();
	istringstream ss(esm.getFriendly());

	string line_lc;
	smatch found;
	pos_c = 0;

	while(getline(ss, line))
	{
		eraseCarriageReturnChar(line);

		found_key = false;
		line_lc = line;
		line_new = "N\\A";

		transform(line_lc.begin(), line_lc.end(),
			  line_lc.begin(), ::tolower);

		if(found_key == false)
		{
			pos = line_lc.find("messagebox");
			convertLine(id, type);
		}

		if(found_key == false)
		{
			pos = line_lc.find("choice");
			convertLine(id, type);
		}

		if(found_key == false)
		{
			pos = line_lc.find("say ");
			convertLine(id, type, true);
		}

		if(found_key == false)
		{
			pos = line_lc.find("say,");
			convertLine(id, type, true);
		}

		if(found_key == false)
		{
			pos = line_lc.find("addtopic");
			convertText("DIAL", yampt::r_type::DIAL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("showmap");
			convertText("CELL", yampt::r_type::CELL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("centeroncell");
			convertText("CELL", yampt::r_type::CELL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("getpccell");
			convertText("CELL", yampt::r_type::CELL, 0, true);
		}

		if(found_key == false)
		{
			pos = line_lc.find("aifollowcell");
			convertText("CELL", yampt::r_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("aiescortcell)");
			convertText("CELL", yampt::r_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("placeitemcell");
			convertText("CELL", yampt::r_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("positioncell");
			convertText("CELL", yampt::r_type::CELL, 4);
		}

		if(found_key == true)
		{
			new_friendly += line_new + "\r\n";
		}
		else
		{
			new_friendly += line + "\r\n";
		}
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
		counter_unchanged++;

		new_friendly = "N\\A";
	}
}

//----------------------------------------------------------
void EsmConverter::convertLine(string id, yampt::r_type type, bool say)
{
	if(pos != string::npos &&
	   line.rfind(";", pos) == string::npos)
	{
		auto search = merger->getDict()[type].find(id + yampt::sep[0] + line);
		if(search != merger->getDict()[type].end())
		{
			if(line != search->second)
			{
				line_new = search->second;

				vector<string> str_list_u = splitLine(line, say);
				vector<string> str_list_f = splitLine(line_new, say);

				if(str_list_u.size() == str_list_f.size())
				{
					for(size_t i = 0; i < str_list_u.size(); i++)
					{
						pos_c = compiled.find(str_list_u[i], pos_c);
						if(pos_c != string::npos)
						{
							pos_c -= 2;
							compiled.erase(pos_c, 2);
							compiled.insert(pos_c, convertIntToByteArray(str_list_f[i].size()).substr(0, 2));
							pos_c += 2;
							compiled.erase(pos_c, str_list_u[i].size());
							compiled.insert(pos_c, str_list_f[i]);
							pos_c += str_list_f[i].size();
						}
					}
				}

				found_key = true;
				result_ptr = &yampt::result[1];
			}
			else
			{
				result_ptr = &yampt::result[2];
			}
		}
		else
		{
			result_ptr = &yampt::result[0];
		}
		makeLogScript(esm.getUnique());
	}
}

//----------------------------------------------------------
void EsmConverter::convertText(string id, yampt::r_type type, int num, bool getpccell)
{
	if(pos != string::npos &&
	   line.rfind(";", pos) == string::npos)
	{
		extractText(num);
		auto search = merger->getDict()[type].find(id + yampt::sep[0] + text);

		if(search != merger->getDict()[type].end())
		{
			line_new = line;
			line_new.erase(pos, text.size());

			pos_c = compiled.find(text, pos_c);
			if(pos_c != string::npos)
			{
				pos_c -= 1;
				compiled.erase(pos_c, 1);
				compiled.insert(pos_c, convertIntToByteArray(search->second.size()).substr(0, 1));
				pos_c += 1;
				compiled.erase(pos_c, text.size());
				compiled.insert(pos_c, search->second);
				if(getpccell == 1)
				{
					size_t size = search->second.size() + 12;
					pos_c -= 8;
					compiled.erase(pos_c, 1);
					compiled.insert(pos_c, convertIntToByteArray(size).substr(0, 1));
					pos_c += size;
				}
				else
				{
					pos_c += search->second.size();
				}
			}

			if(line_new.substr(pos - 1, 1) == "\"")
			{
				line_new.insert(pos, search->second);
			}
			else
			{
				line_new.insert(pos, "\"" + search->second + "\"");
			}

			found_key = true;

			if(line != line_new)
			{
				result_ptr = &yampt::result[1];
			}
			else
			{
				result_ptr = &yampt::result[2];
			}
		}
		else
		{
			result_ptr = &yampt::result[0];
			for(auto &elem : merger->getDict()[type])
			{
				if(caseInsensitiveStringCmp(id + yampt::sep[0] + text, elem.first) == true)
				{
					line_new = line;
					line_new.erase(pos, text.size());

					pos_c = compiled.find(text, pos_c);
					if(pos_c != string::npos)
					{
						pos_c -= 1;
						compiled.erase(pos_c, 1);
						compiled.insert(pos_c, convertIntToByteArray(elem.second.size()).substr(0, 1));
						pos_c += 1;
						compiled.erase(pos_c, text.size());
						compiled.insert(pos_c, elem.second);
						if(getpccell == 1)
						{
							size_t size = elem.second.size() + 12;
							pos_c -= 8;
							compiled.erase(pos_c, 1);
							compiled.insert(pos_c, convertIntToByteArray(size).substr(0, 1));
							pos_c += size;
						}
						else
						{
							pos_c += elem.second.size();
						}
					}

					if(line_new.substr(pos - 1, 1) == "\"")
					{
						line_new.insert(pos, elem.second);
					}
					else
					{
						line_new.insert(pos, "\"" + elem.second + "\"");
					}

					found_key = true;

					if(line != line_new)
					{
						result_ptr = &yampt::result[1];
						break;
					}
					else
					{
						result_ptr = &yampt::result[2];
						break;
					}
				}
			}
		}
		makeLogScript(esm.getUnique());
	}
}

//----------------------------------------------------------
void EsmConverter::extractText(int num)
{
	size_t list_pos;
	string list_var;
	smatch found;
	int ctr = -1;

	list_pos = line.find_first_of(" \t,\"", pos);
	list_pos = line.find_first_not_of(" \t,", list_pos);
	if(list_pos != string::npos)
	{
		list_var = line.substr(list_pos);
	}
	else
	{
		list_pos = 0;
	}

	//cout << "----" << endl;
	//cout << "Line: " << line << endl;
	//cout << "List: " << list_var << endl;

	if(num == 0)
	{
		text = list_var;
		if(text.find("=") != string::npos)
		{
			text.erase(text.find("="));
		}
		if(text.find_last_not_of(" \t") != string::npos)
		{
			text.erase(text.find_last_not_of(" \t") + 1);
		}
		pos = list_pos;
	}
	else
	{
		regex r1("(([\\w\\.]+)|(\".*?\"))");

		sregex_iterator next(list_var.begin(), list_var.end(), r1);
		sregex_iterator end;
		while(next != end && ctr != num)
		{
			found = *next;
			text = found[1].str();
			pos = found.position(1) + list_pos;

			//cout << "Var " << pos << ": " << text << endl;

			next++;
			ctr++;
		}
	}

	regex r2("\"(.*?)\"");
	regex_search(text, found, r2);

	if(!found.empty())
	{
		text = found[1].str();
		pos += 1;
	}

	//cout << "Out " << pos << ": " << text << endl;
}

//----------------------------------------------------------
vector<string> EsmConverter::splitLine(string line, bool say)
{
	vector<string> list_vec;
	string list_str = line.substr(pos);

	smatch found;
	regex re("\"(.*?)\"");
	sregex_iterator next(list_str.begin(), list_str.end(), re);
	sregex_iterator end;
	while(next != end)
	{
		found = *next;
		list_vec.push_back(found[1].str());
		next++;
	}

	if(say == 1 && list_vec.size() > 0)
	{
		list_vec.erase(list_vec.begin());
	}

	for(auto const &elem : list_vec)
	{
		cout << elem << endl << endl;
	}

	return list_vec;
}

//----------------------------------------------------------
void EsmConverter::addDIALtoINFO()
{
	string key;
	string new_friendly_lc;
	size_t pos;

	new_friendly = esm.getFriendly();
	new_friendly_lc = esm.getFriendly();
	transform(new_friendly_lc.begin(), new_friendly_lc.end(),
		  new_friendly_lc.begin(), ::tolower);

	for(const auto &elem : merger->getDict()[yampt::r_type::DIAL])
	{
		key = elem.first.substr(5);
		transform(key.begin(), key.end(),
			  key.begin(), ::tolower);

		if(key != elem.second)
		{
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
				makeLog("CELL" + yampt::sep[0] + esm.getUnique());
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
			makeLog("PGRD " + esm.getUnique());
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
				makeLog("ANAM " + esm.getUnique());
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
					makeLog("SCVR " + esm.getUnique());
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
				makeLog("DNAM " + esm.getUnique());
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
				makeLog("CNDT " + esm.getUnique());
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
				makeLog("GMST" + yampt::sep[0] + esm.getUnique());
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
				makeLog("FNAM" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique());
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
				makeLog("DESC" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique());
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
				makeLog("TEXT" + yampt::sep[0] + esm.getUnique());
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
					makeLog("RNAM" + yampt::sep[0] + esm.getUnique() + yampt::sep[0] + to_string(esm.getFriendlyCounter()));
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
				makeLog("INDX" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique());
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

			if(esm.getUniqueStatus() == true && esm.getFriendlyStatus() == true && esm.getUnique() == "T")
			{
				unique_key = "DIAL" + yampt::sep[0] + esm.getFriendly();
				setNewFriendly(yampt::r_type::DIAL);
				makeLog("DIAL" + yampt::sep[0] + esm.getFriendly());
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
				makeLog("INFO" + yampt::sep[0] + current_dialog + yampt::sep[0] + esm.getUnique());
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
			esm.setUnique("SCHD", false);

			esm.setFriendly("SCDT", false, false);
			compiled = esm.getFriendly();

			esm.setFriendly("SCTX");

			if(esm.getFriendlyStatus() == true)
			{
				setNewFriendlyScript("SCTX", yampt::r_type::SCTX);
				if(convert == true)
				{
					convertRecordContent(new_friendly);

					esm.setFriendly("SCDT", false, false);
					convertRecordContent(compiled);

					esm.setFriendly("SCHD", false, false);
					new_friendly = esm.getFriendly();
					new_friendly.erase(44, 4);
					new_friendly.insert(44, convertIntToByteArray(compiled.size()));
					convertRecordContent(new_friendly);
				}
			}
		}
	}
	printLog("SCTX");
}

//----------------------------------------------------------
void EsmConverter::convertGMDT()
{
	resetCounters();
	string prefix;
	string suffix;

	for(size_t i = 0; i < esm.getRecColl().size(); ++i)
	{
		esm.setRec(i);
		if(esm.getRecId() == "TES3")
		{
			esm.setUnique("GMDT", false);
			esm.setFriendly("GMDT", false, false);

			if(esm.getUniqueStatus() == true)
			{
				unique_key = esm.getUnique().substr(24, 64);
				eraseNullChars(unique_key);
				unique_key = "CELL" + yampt::sep[0] + unique_key;

				prefix = esm.getFriendly().substr(0, 24);
				suffix = esm.getFriendly().substr(88);

				setNewFriendly(yampt::r_type::CELL);
				if(convert == true)
				{
					new_friendly.resize(64);
					convertRecordContent(prefix + new_friendly + suffix);
				}
			}
		}
                if(esm.getRecId() == "GAME")
		{
			esm.setUnique("GMDT", false);
			esm.setFriendly("GMDT", false, false);

			if(esm.getUniqueStatus() == true)
			{
				unique_key = esm.getUnique().substr(0, 64);
				eraseNullChars(unique_key);
				unique_key = "CELL" + yampt::sep[0] + unique_key;

				suffix = esm.getFriendly().substr(64);

				setNewFriendly(yampt::r_type::CELL);
				if(convert == true)
				{
					new_friendly.resize(64);
					convertRecordContent(new_friendly + suffix);
				}
			}
		}
	}
	printLog("GMDT");
}
