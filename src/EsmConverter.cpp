#include "EsmConverter.hpp"

//----------------------------------------------------------
EsmConverter::EsmConverter(std::string path, DictMerger &merger, bool safe, bool add_dial)
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

        std::cout << "----------------------------------------------" << std::endl;
	}
}

//----------------------------------------------------------
void EsmConverter::makeLogHeader()
{
	log += "<!-- Converting " + esm.getName() + "... -->\r\n";
    log += yampt::sep_line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLog(std::string key)
{
	log += "<!-- " + *converter_log_ptr + " '" + key + "' '" + esm.getName() + "' -->\r\n";
	log += esm.getFriendly() + " <!-- >>> -->" + "\r\n";
	log += new_friendly + "\r\n";
    log += yampt::sep_line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLogScript(std::string key)
{
	log += "<!-- " + *converter_log_ptr + " script line '" + key + "' '" + esm.getName() + "' -->\r\n";
	log += line + " <!-- >>> -->" + "\r\n";
	log += line_new + "\r\n";
    log += yampt::sep_line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::printLogHeader()
{
    std::cout << "----------------------------------------------" << std::endl
         << "      Converted / Skipped / Unchanged /    All" << std::endl
         << "----------------------------------------------" << std::endl;
}

//----------------------------------------------------------
void EsmConverter::printLog(std::string id)
{
	if(id == "INFO" && add_dial == true)
	{
		if(counter_add > 0)
		{
            std::cout << std::endl;
		}

        std::cout << id << " "
             << std::setw(10) << std::to_string(counter_converted) << " / "
             << std::setw(7) << std::to_string(counter_skipped) << " / "
             << std::setw(9) << std::to_string(counter_unchanged) << " / "
             << std::setw(6) << std::to_string(counter_all) << std::endl
		     << "+ Link" << " "
             << std::setw(8) << std::to_string(counter_add) << " / "
             << std::setw(7) << "-" << " / "
             << std::setw(9) << "-" << " / "
             << std::setw(6) << "-" << std::endl;
	}
	else
	{
        std::cout << id << " "
             << std::setw(10) << std::to_string(counter_converted) << " / "
             << std::setw(7) << std::to_string(counter_skipped) << " / "
             << std::setw(9) << std::to_string(counter_unchanged) << " / "
             << std::setw(6) << std::to_string(counter_all) << std::endl;
	}
}

//----------------------------------------------------------
void EsmConverter::writeEsm()
{
	if(status == true)
	{
        std::string name = esm.getName();
        std::ofstream file(name, std::ios::binary);
		for(auto &elem : esm.getRecColl())
		{
			file << elem;
		}
        std::cout << "--> Writing " << name << "...\r\n";
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
void EsmConverter::convertRecordContent(std::string text_new)
{
	size_t rec_size;
    std::string rec_content = esm.getRecContent();

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

		counter_converted++;
	}
	else
	{
        converter_log_ptr = &yampt::converter_log[4];
		counter_unchanged++;
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendly(yampt::rec_type type)
{
	counter_all++;

	auto search = merger->getDict()[type].find(unique_key);
	if(search != merger->getDict()[type].end())
	{
		new_friendly = search->second;

		if(esm.getFriendly() != new_friendly)
		{
			to_convert = true;
            converter_log_ptr = &yampt::converter_log[1];
		}
		else
		{
			to_convert = false;
            converter_log_ptr = &yampt::converter_log[2];
			counter_skipped++;
		}
	}
	else
	{
		to_convert = false;
        converter_log_ptr = &yampt::converter_log[0];
		counter_unchanged++;

		new_friendly = "<N\\A>";
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendlyINFO(yampt::rec_type type)
{
	counter_all++;

	auto search = merger->getDict()[type].find(unique_key);
	if(safe == false && search != merger->getDict()[type].end())
	{
		new_friendly = search->second;

		if(esm.getFriendly() != new_friendly)
		{
			to_convert = true;
            converter_log_ptr = &yampt::converter_log[1];
		}
		else
		{
			to_convert = false;
            converter_log_ptr = &yampt::converter_log[2];
			counter_skipped++;
		}
	}
	else if(add_dial == true && esm.getRecId() == "INFO" && dialog_topic.substr(0, 1) != "V")
	{
		addDIALtoINFO();

		if(esm.getFriendly() != new_friendly)
		{
			to_convert = true;
            converter_log_ptr = &yampt::converter_log[3];

			if(counter_add == 1)
			{
                std::cout << "Adding hyperlinks to INFO strings in progress...";
			}
			counter_add++;
			if(counter_add % 200 == 0)
			{
                std::cout << "." << std::flush;
			}
		}
		else
		{
			to_convert = false;
            converter_log_ptr = &yampt::converter_log[0];
			counter_unchanged++;

			new_friendly = "<N\\A>";
		}
	}
	else
	{
		to_convert = false;
        converter_log_ptr = &yampt::converter_log[0];
		counter_unchanged++;

		new_friendly = "<N\\A>";
	}
}

//----------------------------------------------------------
void EsmConverter::addDIALtoINFO()
{
    std::string key;
    std::string new_friendly_lc;
	size_t pos;

	new_friendly = esm.getFriendly();
	new_friendly_lc = esm.getFriendly();
	transform(new_friendly_lc.begin(), new_friendly_lc.end(),
		  new_friendly_lc.begin(), ::tolower);

    for(const auto &elem : merger->getDict()[yampt::rec_type::DIAL])
	{
		key = elem.first.substr(5);
		transform(key.begin(), key.end(),
			  key.begin(), ::tolower);

		if(key != elem.second)
		{
			pos = new_friendly_lc.find(key);
            if(pos != std::string::npos)
			{
				new_friendly.insert(new_friendly.size(), " [" + elem.second + "]");
			}
		}
	}
}

//----------------------------------------------------------
void EsmConverter::setNewFriendlyScript(std::string id, yampt::rec_type type)
{
	counter_all++;
	new_friendly.erase();
    std::istringstream ss(esm.getFriendly());

    std::string line_lc;
    std::smatch found;
	pos_c = 0;

    while(std::getline(ss, line))
	{
		eraseCarriageReturnChar(line);

		found_key = false;
		line_lc = line;
		line_new = "<N\\A>";

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
            convertText("DIAL", yampt::rec_type::DIAL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("showmap");
            convertText("CELL", yampt::rec_type::CELL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("centeroncell");
            convertText("CELL", yampt::rec_type::CELL, 0);
		}

		if(found_key == false)
		{
			pos = line_lc.find("getpccell");
            convertText("CELL", yampt::rec_type::CELL, 0, true); // is_getpccell_keyword = true
		}

		if(found_key == false)
		{
			pos = line_lc.find("aifollowcell");
            convertText("CELL", yampt::rec_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("aiescortcell)");
            convertText("CELL", yampt::rec_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("placeitemcell");
            convertText("CELL", yampt::rec_type::CELL, 1);
		}

		if(found_key == false)
		{
			pos = line_lc.find("positioncell");
            convertText("CELL", yampt::rec_type::CELL, 4);
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
    if(last_nl_pos != esm.getFriendly().size() - 2 || last_nl_pos == std::string::npos)
	{
		new_friendly.resize(new_friendly.size() - 2);
	}

	if(esm.getFriendly() != new_friendly)
	{
		to_convert = true;
        converter_log_ptr = &yampt::converter_log[1];
	}
	else
	{
		to_convert = false;
        converter_log_ptr = &yampt::converter_log[0];
		counter_unchanged++;

		new_friendly = "<N\\A>";
	}
}

//----------------------------------------------------------
void EsmConverter::convertLine(std::string id, yampt::rec_type type, bool is_say_keyword)
{
    if(pos != std::string::npos &&
       line.rfind(";", pos) == std::string::npos)
	{
        auto search = merger->getDict()[type].find(id + yampt::sep[0] + line);
		if(search != merger->getDict()[type].end())
		{
			if(line != search->second)
			{
				line_new = search->second;

				convertLineCompiledScriptData(is_say_keyword);

				found_key = true;
                converter_log_ptr = &yampt::converter_log[1];
			}
			else
			{
                converter_log_ptr = &yampt::converter_log[2];
			}
		}
		else
		{
            converter_log_ptr = &yampt::converter_log[0];
		}
		makeLogScript(esm.getUnique());
	}
}

//----------------------------------------------------------
void EsmConverter::convertLineCompiledScriptData(bool is_say_keyword)
{
    std::vector<std::string> str_list_u = splitLine(line, is_say_keyword);
    std::vector<std::string> str_list_f = splitLine(line_new, is_say_keyword);

	if(str_list_u.size() == str_list_f.size())
	{
		for(size_t i = 0; i < str_list_u.size(); i++)
		{
			pos_c = compiled_data.find(str_list_u[i], pos_c);
            if(pos_c != std::string::npos)
			{
				if(i == 0)
				{
					pos_c -= 2;
					compiled_data.erase(pos_c, 2);
					compiled_data.insert(pos_c, convertIntToByteArray(str_list_f[i].size()).substr(0, 2));
					pos_c += 2;
					compiled_data.erase(pos_c, str_list_u[i].size());
					compiled_data.insert(pos_c, str_list_f[i]);
					pos_c += str_list_f[i].size();
				}
				else
				{
					pos_c -= 1;
					compiled_data.erase(pos_c, 1);
					compiled_data.insert(pos_c, convertIntToByteArray(str_list_f[i].size() + 1).substr(0, 1));
					pos_c += 1;
					compiled_data.erase(pos_c, str_list_u[i].size());
					compiled_data.insert(pos_c, str_list_f[i]);
					pos_c += str_list_f[i].size();
				}
			}
		}
	}
}

//----------------------------------------------------------
void EsmConverter::convertText(std::string id, yampt::rec_type type, int num, bool is_getpccell_keyword)
{
    if(pos != std::string::npos &&
       line.rfind(";", pos) == std::string::npos)
	{
		extractText(num);
        auto search = merger->getDict()[type].find(id + yampt::sep[0] + text);

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
                converter_log_ptr = &yampt::converter_log[1];
			}
			else
			{
                converter_log_ptr = &yampt::converter_log[2];
			}
		}
		else // Slow search case aware
		{
            converter_log_ptr = &yampt::converter_log[0];
			for(auto &elem : merger->getDict()[type])
			{
                if(caseInsensitiveStringCmp(id + yampt::sep[0] + text, elem.first) == true)
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
                        converter_log_ptr = &yampt::converter_log[1];
						break;
					}
					else
					{
                        converter_log_ptr = &yampt::converter_log[2];
						break;
					}
				}
			}
		}
		makeLogScript(esm.getUnique());
	}
}

//----------------------------------------------------------
void EsmConverter::convertTextCompiledScriptData(std::string text_new, bool is_getpccell_keyword)
{
	pos_c = compiled_data.find(text, pos_c);
    if(pos_c != std::string::npos)
	{
		pos_c -= 1;
		compiled_data.erase(pos_c, 1);
		compiled_data.insert(pos_c, convertIntToByteArray(text_new.size()).substr(0, 1));
		pos_c += 1;
		compiled_data.erase(pos_c, text.size());
		compiled_data.insert(pos_c, text_new);

		if(is_getpccell_keyword == true)
		{
			size_t size = text_new.size() + 12;
			pos_c -= 8;
			compiled_data.erase(pos_c, 1);
			compiled_data.insert(pos_c, convertIntToByteArray(size).substr(0, 1));
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
    std::string list_var;
    std::smatch found;
	int ctr = -1;

	list_pos = line.find_first_of(" \t,\"", pos);
	list_pos = line.find_first_not_of(" \t,", list_pos);
    if(list_pos != std::string::npos)
	{
		list_var = line.substr(list_pos);
	}
	else
	{
		list_pos = 0;
	}

    //std::cout << "----" << std::endl;
    //std::cout << "Line: " << line << std::endl;
    //std::cout << "List: " << list_var << std::endl;

	if(num == 0)
	{
		text = list_var;
        if(text.find("=") != std::string::npos)
		{
			text.erase(text.find("="));
		}
        if(text.find_last_not_of(" \t") != std::string::npos)
		{
			text.erase(text.find_last_not_of(" \t") + 1);
		}
		pos = list_pos;
	}
	else
	{
        std::regex r1("(([\\w\\.]+)|(\".*?\"))");

        std::sregex_iterator next(list_var.begin(), list_var.end(), r1);
        std::sregex_iterator end;
		while(next != end && ctr != num)
		{
			found = *next;
			text = found[1].str();
			pos = found.position(1) + list_pos;

            //std::cout << "Var " << pos << ": " << text << std::endl;

			next++;
			ctr++;
		}
	}

    std::regex r2("\"(.*?)\"");
    std::regex_search(text, found, r2);

	if(!found.empty())
	{
		text = found[1].str();
		pos += 1;
	}

    //std::cout << "Out " << pos << ": " << text << std::endl;
}

//----------------------------------------------------------
std::vector<std::string> EsmConverter::splitLine(std::string line, bool is_say_keyword)
{
    std::vector<std::string> list_vec;
    std::string list_str = line.substr(pos);

    std::smatch found;
    std::regex re("\"(.*?)\"");
    std::sregex_iterator next(list_str.begin(), list_str.end(), re);
    std::sregex_iterator end;
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
        std::cout << elem << endl << std::endl;
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
                unique_key = "CELL" + yampt::sep[0] + esm.getUnique();
                setNewFriendly(yampt::rec_type::CELL);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("CELL" + yampt::sep[0] + esm.getUnique());
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
                unique_key = "CELL" + yampt::sep[0] + esm.getUnique();
                setNewFriendly(yampt::rec_type::CELL);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("PGRD" + yampt::sep[0] + esm.getUnique());
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
                setNewFriendly(yampt::rec_type::CELL);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("ANAM" + yampt::sep[0] + esm.getUnique());
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
                    setNewFriendly(yampt::rec_type::CELL);
					new_friendly = esm.getFriendly().substr(0, 5) + new_friendly;

					if(to_convert == true)
					{
						convertRecordContent(new_friendly);
					}

                    makeLog("SCVR" + yampt::sep[0] + esm.getUnique());
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
                unique_key = "CELL" + yampt::sep[0] + esm.getFriendly();
                setNewFriendly(yampt::rec_type::CELL);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');

				}

                makeLog("DNAM" + yampt::sep[0] + esm.getUnique());

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
                unique_key = "CELL" + yampt::sep[0] + esm.getFriendly();
                setNewFriendly(yampt::rec_type::CELL);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("CNDT" + yampt::sep[0] + esm.getUnique());

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
                unique_key = "GMST" + yampt::sep[0] + esm.getUnique();
                setNewFriendly(yampt::rec_type::GMST);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("GMST" + yampt::sep[0] + esm.getUnique());
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
                unique_key = "FNAM" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique();
                setNewFriendly(yampt::rec_type::FNAM);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("FNAM" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique());
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
                unique_key = "DESC" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique();
                setNewFriendly(yampt::rec_type::DESC);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("DESC" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique());
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
                unique_key = "TEXT" + yampt::sep[0] + esm.getUnique();
                setNewFriendly(yampt::rec_type::TEXT);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("TEXT" + yampt::sep[0] + esm.getUnique());
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
                    unique_key = "RNAM" + yampt::sep[0] + esm.getUnique() + yampt::sep[0] + std::to_string(esm.getFriendlyCounter());
                    setNewFriendly(yampt::rec_type::RNAM);

					if(to_convert == true)
					{
						new_friendly.resize(32);
						convertRecordContent(new_friendly);
					}

                    makeLog("RNAM" + yampt::sep[0] + esm.getUnique() + yampt::sep[0] + std::to_string(esm.getFriendlyCounter()));

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
                unique_key = "INDX" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique();
                setNewFriendly(yampt::rec_type::INDX);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("INDX" + yampt::sep[0] + esm.getRecId() + yampt::sep[0] + esm.getUnique());
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
                unique_key = "DIAL" + yampt::sep[0] + esm.getFriendly();
                setNewFriendly(yampt::rec_type::DIAL);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("DIAL" + yampt::sep[0] + esm.getFriendly());
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
                dialog_topic = esm.getUnique() + yampt::sep[0] + esm.getFriendly();
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
                unique_key = "INFO" + yampt::sep[0] + dialog_topic + yampt::sep[0] + esm.getUnique();
                setNewFriendlyINFO(yampt::rec_type::INFO);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly + '\0');
				}

                makeLog("INFO" + yampt::sep[0] + dialog_topic + yampt::sep[0] + esm.getUnique());
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
                setNewFriendlyScript("BNAM", yampt::rec_type::BNAM);

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
			compiled_data = esm.getFriendly();

			esm.setFriendly("SCTX");

			if(esm.getFriendlyStatus() == true)
			{
                setNewFriendlyScript("SCTX", yampt::rec_type::SCTX);

				if(to_convert == true)
				{
					convertRecordContent(new_friendly);

					esm.setFriendly("SCDT", false);
					convertRecordContent(compiled_data);

					esm.setFriendly("SCHD", false);
					new_friendly = esm.getFriendly();
					new_friendly.erase(44, 4);
					new_friendly.insert(44, convertIntToByteArray(compiled_data.size()));
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
    std::string prefix;
    std::string suffix;

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
                unique_key = "CELL" + yampt::sep[0] + unique_key;

				prefix = esm.getFriendly().substr(0, 24);
				suffix = esm.getFriendly().substr(88);

                setNewFriendly(yampt::rec_type::CELL);

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
                unique_key = "CELL" + yampt::sep[0] + unique_key;

				suffix = esm.getFriendly().substr(64);

                setNewFriendly(yampt::rec_type::CELL);

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
