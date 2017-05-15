#include "EsmConverter.hpp"

using namespace std;
using namespace yampt;

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
		printLogHeader();
		makeLogHeader();

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

		cout << "----------------------------------------------" << endl;
	}
}

//----------------------------------------------------------
void EsmConverter::makeLogHeader()
{
	log += "<!-- Converting " + esm.getName() + "... -->\r\n";
	log += yampt::line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLog(string key)
{
	log += "<!-- " + *converter_log_ptr + " '" + key + "' -->\r\n";
	if(converter_log_ptr == &converter_log[0] || converter_log_ptr == &converter_log[2])
        {
  		log += esm.getFriendly() + "\r\n";
        }
        else
        {
		log += esm.getFriendly() + " <!-- >>> -->" + "\r\n";
		log += new_friendly + "\r\n";
	}
	log += yampt::line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLogScript(string key)
{
	log += "<!-- " + *converter_log_ptr + " script line '" + key + "' -->\r\n";
	log += line + " <!-- >>> -->" + "\r\n";
	log += line_new + "\r\n";
	log += yampt::line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::printLogHeader()
{
	cout << "----------------------------------------------" << endl
	     << "      Converted / Skipped / Unchanged /    All" << endl
	     << "----------------------------------------------" << endl;
}

//----------------------------------------------------------
void EsmConverter::printLog(string id)
{
	if(id == "INFO" && add_dial == true)
	{
		if(counter_add > 0)
		{
			cout << endl;
		}

		cout << id << " "
		     << setw(10) << to_string(counter_converted) << " / "
		     << setw(7) << to_string(counter_skipped) << " / "
		     << setw(9) << to_string(counter_unchanged) << " / "
		     << setw(6) << to_string(counter_all) << endl
		     << "+ Link" << " "
		     << setw(8) << to_string(counter_add) << " / "
		     << setw(7) << "-" << " / "
		     << setw(9) << "-" << " / "
		     << setw(6) << "-" << endl;
	}
	else
	{
		cout << id << " "
		     << setw(10) << to_string(counter_converted) << " / "
		     << setw(7) << to_string(counter_skipped) << " / "
		     << setw(9) << to_string(counter_unchanged) << " / "
		     << setw(6) << to_string(counter_all) << endl;
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
	counter_skipped = 0;
	counter_unchanged = 0;
	counter_all = 0;
	counter_add = 0;
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent(string text_new)
{
	size_t rec_size;
	string rec_content = esm.getRecContent();

	if(esm.getFriendlyStatus() == true)
	{
		rec_content.erase(esm.getFriendlyPos() + 8, esm.getFriendlySize());
		rec_content.insert(esm.getFriendlyPos() + 8, text_new);
		rec_content.erase(esm.getFriendlyPos() + 4, 4);
		rec_content.insert(esm.getFriendlyPos() + 4, convertIntToByteArray(text_new.size()));
		rec_size = rec_content.size() - 16;
		rec_content.erase(4, 4);
		rec_content.insert(4, convertIntToByteArray(rec_size));
		esm.setRecContent(rec_content);
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendly(r_type type)
{
	counter_all++;

	auto search = merger->getDict()[type].find(unique_key);
	if(search != merger->getDict()[type].end())
	{
		new_friendly = search->second;

		if(esm.getFriendly() != new_friendly)
		{
			to_convert = true;
			converter_log_ptr = &converter_log[1];
			counter_converted++;
		}
		else
		{
			to_convert = false;
			converter_log_ptr = &converter_log[2];
			counter_skipped++;
		}
	}
	else
	{
		to_convert = false;
		converter_log_ptr = &converter_log[0];
		counter_unchanged++;

		new_friendly = "N\\A";
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendlyINFO(r_type type)
{
	counter_all++;

	auto search = merger->getDict()[type].find(unique_key);
	if(safe == false && search != merger->getDict()[type].end())
	{
		new_friendly = search->second;

		if(esm.getFriendly() != new_friendly)
		{
			to_convert = true;
			converter_log_ptr = &converter_log[1];
			counter_converted++;
		}
		else
		{
			to_convert = false;
			converter_log_ptr = &converter_log[2];
			counter_skipped++;
		}
	}
	else if(add_dial == true && esm.getRecId() == "INFO" && dialog_topic.substr(0, 1) != "V")
	{
		addDIALtoINFO();

		if(esm.getFriendly() != new_friendly)
		{
			to_convert = true;
			converter_log_ptr = &converter_log[3];

			if(counter_add == 1)
			{
				cout << "Adding hyperlinks to INFO strings in progress...";
			}
			counter_add++;
			if(counter_add % 200 == 0)
			{
				cout << "." << flush;
			}
		}
		else
		{
			to_convert = false;
			converter_log_ptr = &converter_log[0];
			counter_unchanged++;

			new_friendly = "N\\A";
		}
	}
	else
	{
		to_convert = false;
		converter_log_ptr = &converter_log[0];
		counter_unchanged++;

		new_friendly = "N\\A";
	}
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

	for(const auto &elem : merger->getDict()[r_type::DIAL])
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
void EsmConverter::setNewFriendlyScript(string id, r_type type)
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
			pos = line_lc.find("say ");  // ugly keyword
			convertLine(id, type, true); // is_say_keyword = true
		}

		if(found_key == false)
		{
			pos = line_lc.find("say,");
			convertLine(id, type, true);
		}

		if(found_key == false)
		{
			pos = line_lc.find("addtopic");
			convertText("DIAL", r_type::DIAL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("showmap");
			convertText("CELL", r_type::CELL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("centeroncell");
			convertText("CELL", r_type::CELL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("getpccell");
			convertText("CELL", r_type::CELL, 0, true); // is_getpccell_keyword = true
		}

		if(found_key == false)
		{
			pos = line_lc.find("aifollowcell");
			convertText("CELL", r_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("aiescortcell)");
			convertText("CELL", r_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("placeitemcell");
			convertText("CELL", r_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("positioncell");
			convertText("CELL", r_type::CELL, 4);
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

	// Check if last 2 char are new line
	size_t last_nl_pos = esm.getFriendly().rfind("\r\n");
	if(last_nl_pos != esm.getFriendly().size() - 2 || last_nl_pos == string::npos)
	{
		new_friendly.resize(new_friendly.size() - 2);
	}

	if(esm.getFriendly() != new_friendly)
	{
		to_convert = true;
		converter_log_ptr = &converter_log[1];
		counter_converted++;
	}
	else
	{
		to_convert = false;
		converter_log_ptr = &converter_log[0];
		counter_unchanged++;

		new_friendly = "N\\A";
	}
}

//----------------------------------------------------------
void EsmConverter::convertLine(string id, r_type type, bool is_say_keyword)
{
	if(pos != string::npos &&
	   line.rfind(";", pos) == string::npos)
	{
		auto search = merger->getDict()[type].find(id + sep[0] + line);
		if(search != merger->getDict()[type].end())
		{
			if(line != search->second)
			{
				line_new = search->second;

				convertLineCompiledScriptData(is_say_keyword);

				found_key = true;
				converter_log_ptr = &converter_log[1];
			}
			else
			{
				converter_log_ptr = &converter_log[2];
			}
		}
		else
		{
			converter_log_ptr = &converter_log[0];
		}
		makeLogScript(esm.getUnique());
	}
}

//----------------------------------------------------------
void EsmConverter::convertLineCompiledScriptData(bool is_say_keyword)
{
	vector<string> str_list_u = splitLine(line, is_say_keyword);
	vector<string> str_list_f = splitLine(line_new, is_say_keyword);

	if(str_list_u.size() == str_list_f.size())
	{
		for(size_t i = 0; i < str_list_u.size(); i++)
		{
			pos_c = compiled.find(str_list_u[i], pos_c);
			if(pos_c != string::npos)
			{
				if(i == 0)
				{
					pos_c -= 2;
					compiled.erase(pos_c, 2);
					compiled.insert(pos_c, convertIntToByteArray(str_list_f[i].size()).substr(0, 2));
					pos_c += 2;
					compiled.erase(pos_c, str_list_u[i].size());
					compiled.insert(pos_c, str_list_f[i]);
					pos_c += str_list_f[i].size();
				}
				else
				{
					pos_c -= 1;
					compiled.erase(pos_c, 1);
					compiled.insert(pos_c, convertIntToByteArray(str_list_f[i].size() + 1).substr(0, 1));
					pos_c += 1;
					compiled.erase(pos_c, str_list_u[i].size());
					compiled.insert(pos_c, str_list_f[i]);
					pos_c += str_list_f[i].size();
				}
			}
		}
	}
}

//----------------------------------------------------------
void EsmConverter::convertText(string id, r_type type, int num, bool is_getpccell_keyword)
{
	if(pos != string::npos &&
	   line.rfind(";", pos) == string::npos)
	{
		extractText(num);
		auto search = merger->getDict()[type].find(id + sep[0] + text);

		// Fast search in map
		if(search != merger->getDict()[type].end())
		{
			line_new = line;
			line_new.erase(pos, text.size());

			convertTextCompiledScriptData(search->second, is_getpccell_keyword);

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
				converter_log_ptr = &converter_log[1];
			}
			else
			{
				converter_log_ptr = &converter_log[2];
			}
		}
		else // Slow search case aware
		{
			converter_log_ptr = &converter_log[0];
			for(auto &elem : merger->getDict()[type])
			{
				if(caseInsensitiveStringCmp(id + sep[0] + text, elem.first) == true)
				{
					line_new = line;
					line_new.erase(pos, text.size());

					convertTextCompiledScriptData(elem.second, is_getpccell_keyword);

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
						converter_log_ptr = &converter_log[1];
						break;
					}
					else
					{
						converter_log_ptr = &converter_log[2];
						break;
					}
				}
			}
		}
		makeLogScript(esm.getUnique());
	}
}

//----------------------------------------------------------
void EsmConverter::convertTextCompiledScriptData(string text_new, bool is_getpccell_keyword)
{
	pos_c = compiled.find(text, pos_c);
	if(pos_c != string::npos)
	{
		pos_c -= 1;
		compiled.erase(pos_c, 1);
		compiled.insert(pos_c, convertIntToByteArray(text_new.size()).substr(0, 1));
		pos_c += 1;
		compiled.erase(pos_c, text.size());
		compiled.insert(pos_c, text_new);

		if(is_getpccell_keyword == true)
		{
			size_t size = text_new.size() + 12;
			pos_c -= 8;
			compiled.erase(pos_c, 1);
			compiled.insert(pos_c, convertIntToByteArray(size).substr(0, 1));
			pos_c += size;
		}
		else
		{
			pos_c += text_new.size();
		}
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
vector<string> EsmConverter::splitLine(string line, bool is_say_keyword)
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

	if(is_say_keyword == true && list_vec.size() > 0)
	{
		list_vec.erase(list_vec.begin());
	}

	/*for(auto const &elem : list_vec)
	{
		cout << elem << endl << endl;
	}*/

	return list_vec;
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
				unique_key = "CELL" + sep[0] + esm.getUnique();
				setNewFriendly(r_type::CELL);
				makeLog("CELL" + sep[0] + esm.getUnique());

				if(to_convert == true)
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

			if(esm.getUniqueStatus() == true)
			{
				unique_key = "CELL" + sep[0] + esm.getUnique();
				setNewFriendly(r_type::CELL);
				makeLog("PGRD " + esm.getUnique());

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}
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
				unique_key = "CELL" + sep[0] + esm.getUnique();
				setNewFriendly(r_type::CELL);
				makeLog("ANAM " + esm.getUnique());

				if(to_convert == true)
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
					unique_key = "CELL" + sep[0] + esm.getFriendly().substr(5);
					setNewFriendly(r_type::CELL);
					new_friendly = esm.getFriendly().substr(0, 5) + new_friendly;
					makeLog("SCVR " + esm.getUnique());

					if(to_convert == true)
					{
						convertRecordContent(new_friendly);
					}
				}

				esm.setFriendly("SCVR", true, true);
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
				unique_key = "CELL" + sep[0] + esm.getFriendly();
				setNewFriendly(r_type::CELL);
				makeLog("DNAM " + esm.getUnique());

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');

				}

				esm.setFriendly("DNAM", true, true);
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
				unique_key = "CELL" + sep[0] + esm.getFriendly();
				setNewFriendly(r_type::CELL);
				makeLog("CNDT " + esm.getUnique());

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

				esm.setFriendly("CNDT", true, true);
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

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true &&
			   esm.getUnique().substr(0, 1) == "s")
			{
				unique_key = "GMST" + sep[0] + esm.getUnique();
				setNewFriendly(r_type::GMST);
				makeLog("GMST" + sep[0] + esm.getUnique());

				if(to_convert == true)
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

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true &&
			   esm.getUnique() != "player")
			{
				unique_key = "FNAM" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();
				setNewFriendly(r_type::FNAM);
				makeLog("FNAM" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique());

				if(to_convert == true)
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

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true)
			{
				unique_key = "DESC" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();
				setNewFriendly(r_type::DESC);
				makeLog("DESC" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique());

				if(to_convert == true)
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

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true)
			{
				unique_key = "TEXT" + sep[0] + esm.getUnique();
				setNewFriendly(r_type::TEXT);
				makeLog("TEXT" + sep[0] + esm.getUnique());

				if(to_convert == true)
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
					unique_key = "RNAM" + sep[0] + esm.getUnique() + sep[0] + to_string(esm.getFriendlyCounter());
					setNewFriendly(r_type::RNAM);
					makeLog("RNAM" + sep[0] + esm.getUnique() + sep[0] + to_string(esm.getFriendlyCounter()));

					if(to_convert == true)
					{
						new_friendly.resize(32);
						convertRecordContent(new_friendly);
					}

					esm.setFriendly("RNAM", true, true);
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

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true)
			{
				unique_key = "INDX" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique();
				setNewFriendly(r_type::INDX);
				makeLog("INDX" + sep[0] + esm.getRecId() + sep[0] + esm.getUnique());

				if(to_convert == true)
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

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true &&
			   esm.getUnique() == "T")
			{
				unique_key = "DIAL" + sep[0] + esm.getFriendly();
				setNewFriendly(r_type::DIAL);
				makeLog("DIAL" + sep[0] + esm.getFriendly());

				if(to_convert == true)
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

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true)
			{
				dialog_topic = esm.getUnique() + sep[0] + esm.getFriendly();
			}
			else
			{
				dialog_topic = "<NOTFOUND>";
			}
		}

		if(esm.getRecId() == "INFO")
		{
			esm.setUnique("INAM");
			esm.setFriendly("NAME");

			if(esm.getUniqueStatus() == true &&
			   esm.getFriendlyStatus() == true)
			{
				unique_key = "INFO" + sep[0] + dialog_topic + sep[0] + esm.getUnique();
				setNewFriendlyINFO(r_type::INFO);
				makeLog("INFO" + sep[0] + dialog_topic + sep[0] + esm.getUnique());

				if(to_convert == true)
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
				setNewFriendlyScript("BNAM", r_type::BNAM);

				if(to_convert == true)
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

			esm.setFriendly("SCDT", false);
			compiled = esm.getFriendly();

			esm.setFriendly("SCTX");

			if(esm.getFriendlyStatus() == true)
			{
				setNewFriendlyScript("SCTX", r_type::SCTX);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly);

					esm.setFriendly("SCDT", false);
					convertRecordContent(compiled);

					esm.setFriendly("SCHD", false);
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
			esm.setFriendly("GMDT", false);

			if(esm.getUniqueStatus() == true)
			{
				unique_key = esm.getUnique().substr(24, 64);
				eraseNullChars(unique_key);
				unique_key = "CELL" + sep[0] + unique_key;

				prefix = esm.getFriendly().substr(0, 24);
				suffix = esm.getFriendly().substr(88);

				setNewFriendly(r_type::CELL);

				if(to_convert == true)
				{
					new_friendly.resize(64);
					convertRecordContent(prefix + new_friendly + suffix);
				}
			}
		}

                if(esm.getRecId() == "GAME")
		{
			esm.setUnique("GMDT", false);
			esm.setFriendly("GMDT", false);

			if(esm.getUniqueStatus() == true)
			{
				unique_key = esm.getUnique().substr(0, 64);
				eraseNullChars(unique_key);
				unique_key = "CELL" + sep[0] + unique_key;

				suffix = esm.getFriendly().substr(64);

				setNewFriendly(r_type::CELL);

				if(to_convert == true)
				{
					new_friendly.resize(64);
					convertRecordContent(new_friendly + suffix);
				}
			}
		}
	}

	printLog("GMDT");
}
