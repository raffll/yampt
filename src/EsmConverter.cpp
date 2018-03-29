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
    log += "<!-- Converting " + esm.getNameFull() + "... -->\r\n";
    log += yampt::sep_line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLog(std::string key)
{
    log += "<!-- " + *converter_log_ptr + " '" + key + "' '" + esm.getNameFull() + "' -->\r\n";
    log += esm.getFriendlyText() + " <!-- >>> -->" + "\r\n";
    log += new_friendly + "\r\n";
    log += yampt::sep_line + "\r\n";
}

//----------------------------------------------------------
void EsmConverter::makeLogScript(std::string key)
{
    log += "<!-- " + *converter_log_ptr + " script line '" + key + "' '" + esm.getNameFull() + "' -->\r\n";
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
        std::string name = esm.getNameFull();
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
    std::string rec_content = esm.getRecordContent();

    if(esm.getFriendlyStatus() == true)
    {
        rec_content.erase(esm.getFriendlyPos() + 8, esm.getFriendlySize());
        rec_content.insert(esm.getFriendlyPos() + 8, text_new);
        rec_content.erase(esm.getFriendlyPos() + 4, 4);
        rec_content.insert(esm.getFriendlyPos() + 4, convertIntToByteArray(text_new.size()));
        rec_size = rec_content.size() - 16;
        rec_content.erase(4, 4);
        rec_content.insert(4, convertIntToByteArray(rec_size));
        esm.setNewRecordContent(rec_content);

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

        if(esm.getFriendlyText() != new_friendly)
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

        if(esm.getFriendlyText() != new_friendly)
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
    else if(add_dial == true && esm.getRecordId() == "INFO" && dialog_topic.substr(0, 1) != "V")
    {
        addDIALtoINFO();

        if(esm.getFriendlyText() != new_friendly)
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

    new_friendly = esm.getFriendlyText();
    new_friendly_lc = esm.getFriendlyText();
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
    std::istringstream ss(esm.getFriendlyText());

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
    size_t last_nl_pos = esm.getFriendlyText().rfind("\r\n");
    if(last_nl_pos != esm.getFriendlyText().size() - 2 || last_nl_pos == std::string::npos)
    {
        new_friendly.resize(new_friendly.size() - 2);
    }

    if(esm.getFriendlyText() != new_friendly)
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
        makeLogScript(esm.getUniqueText());
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
        makeLogScript(esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "CELL")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("NAME");

            if(esm.getUniqueStatus() == true)
            {
                unique_key = "CELL" + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::CELL);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("CELL" + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "PGRD")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("NAME");

            if(esm.getUniqueStatus() == true)
            {
                unique_key = "CELL" + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::CELL);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("PGRD" + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("ANAM");
            esm.setFriendlyTo("ANAM");

            if(esm.getUniqueStatus() == true)
            {
                unique_key = "CELL" + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::CELL);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("ANAM" + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFriendlyTo("SCVR");

            while(esm.getFriendlyStatus() == true)
            {
                if(esm.getFriendlyText().substr(1, 1) == "B")
                {
                    unique_key = "CELL" + yampt::sep[0] + esm.getFriendlyText().substr(5);
                    setNewFriendly(yampt::rec_type::CELL);
                    new_friendly = esm.getFriendlyText().substr(0, 5) + new_friendly;

                    if(to_convert == true)
                    {
                        convertRecordContent(new_friendly);
                    }

                    makeLog("SCVR" + yampt::sep[0] + esm.getUniqueText());
                }

                esm.setFriendlyTo("SCVR", true);
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "CELL" ||
           esm.getRecordId() == "NPC_")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("DNAM");

            while(esm.getFriendlyStatus() == true)
            {
                unique_key = "CELL" + yampt::sep[0] + esm.getFriendlyText();
                setNewFriendly(yampt::rec_type::CELL);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');

                }

                makeLog("DNAM" + yampt::sep[0] + esm.getUniqueText());

                esm.setFriendlyTo("DNAM", true);
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "NPC_")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("CNDT");

            while(esm.getFriendlyStatus() == true)
            {
                unique_key = "CELL" + yampt::sep[0] + esm.getFriendlyText();
                setNewFriendly(yampt::rec_type::CELL);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("CNDT" + yampt::sep[0] + esm.getUniqueText());

                esm.setFriendlyTo("CNDT", true);
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "GMST")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("STRV");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText().substr(0, 1) == "s")
            {
                unique_key = "GMST" + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::GMST);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("GMST" + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "ACTI" ||
           esm.getRecordId() == "ALCH" ||
           esm.getRecordId() == "APPA" ||
           esm.getRecordId() == "ARMO" ||
           esm.getRecordId() == "BOOK" ||
           esm.getRecordId() == "BSGN" ||
           esm.getRecordId() == "CLAS" ||
           esm.getRecordId() == "CLOT" ||
           esm.getRecordId() == "CONT" ||
           esm.getRecordId() == "CREA" ||
           esm.getRecordId() == "DOOR" ||
           esm.getRecordId() == "FACT" ||
           esm.getRecordId() == "INGR" ||
           esm.getRecordId() == "LIGH" ||
           esm.getRecordId() == "LOCK" ||
           esm.getRecordId() == "MISC" ||
           esm.getRecordId() == "NPC_" ||
           esm.getRecordId() == "PROB" ||
           esm.getRecordId() == "RACE" ||
           esm.getRecordId() == "REGN" ||
           esm.getRecordId() == "REPA" ||
           esm.getRecordId() == "SKIL" ||
           esm.getRecordId() == "SPEL" ||
           esm.getRecordId() == "WEAP")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("FNAM");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText() != "player")
            {
                unique_key = "FNAM" + yampt::sep[0] + esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::FNAM);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("FNAM" + yampt::sep[0] + esm.getRecordId() + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "BSGN" ||
           esm.getRecordId() == "CLAS" ||
           esm.getRecordId() == "RACE")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("DESC");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_key = "DESC" + yampt::sep[0] + esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::DESC);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("DESC" + yampt::sep[0] + esm.getRecordId() + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "BOOK")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("TEXT");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_key = "TEXT" + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::TEXT);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("TEXT" + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "FACT")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("RNAM");

            if(esm.getUniqueStatus() == true)
            {
                while(esm.getFriendlyStatus() == true)
                {
                    unique_key = "RNAM" + yampt::sep[0] + esm.getUniqueText() + yampt::sep[0] + std::to_string(esm.getFriendlyCounter());
                    setNewFriendly(yampt::rec_type::RNAM);

                    if(to_convert == true)
                    {
                        new_friendly.resize(32);
                        convertRecordContent(new_friendly);
                    }

                    makeLog("RNAM" + yampt::sep[0] + esm.getUniqueText() + yampt::sep[0] + std::to_string(esm.getFriendlyCounter()));

                    esm.setFriendlyTo("RNAM", true);
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "SKIL" ||
           esm.getRecordId() == "MGEF")
        {
            esm.setUniqueTo("INDX");
            esm.setFriendlyTo("DESC");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_key = "INDX" + yampt::sep[0] + esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                setNewFriendly(yampt::rec_type::INDX);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("INDX" + yampt::sep[0] + esm.getRecordId() + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "DIAL")
        {
            esm.setUniqueTo("DATA");
            esm.setFriendlyTo("NAME");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText() == "T")
            {
                unique_key = "DIAL" + yampt::sep[0] + esm.getFriendlyText();
                setNewFriendly(yampt::rec_type::DIAL);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("DIAL" + yampt::sep[0] + esm.getFriendlyText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "DIAL")
        {
            esm.setUniqueTo("DATA");
            esm.setFriendlyTo("NAME");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                dialog_topic = esm.getUniqueText() + yampt::sep[0] + esm.getFriendlyText();
            }
            else
            {
                dialog_topic = "<NOTFOUND>";
            }
        }

        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFriendlyTo("NAME");

            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_key = "INFO" + yampt::sep[0] + dialog_topic + yampt::sep[0] + esm.getUniqueText();
                setNewFriendlyINFO(yampt::rec_type::INFO);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }

                makeLog("INFO" + yampt::sep[0] + dialog_topic + yampt::sep[0] + esm.getUniqueText());
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFriendlyTo("BNAM");

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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "SCPT")
        {
            esm.setUniqueTo("SCHD");

            esm.setFriendlyTo("SCDT", false);
            compiled_data = esm.getFriendlyText();

            esm.setFriendlyTo("SCTX");

            if(esm.getFriendlyStatus() == true)
            {
                setNewFriendlyScript("SCTX", yampt::rec_type::SCTX);

                if(to_convert == true)
                {
                    convertRecordContent(new_friendly);

                    esm.setFriendlyTo("SCDT", false);
                    convertRecordContent(compiled_data);

                    esm.setFriendlyTo("SCHD", false);
                    new_friendly = esm.getFriendlyText();
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
        esm.setRecordTo(i);

        if(esm.getRecordId() == "TES3")
        {
            esm.setUniqueTo("GMDT");
            esm.setFriendlyTo("GMDT");

            if(esm.getUniqueStatus() == true)
            {
                unique_key = esm.getUniqueRaw().substr(24, 64);
                eraseNullChars(unique_key);
                unique_key = "CELL" + yampt::sep[0] + unique_key;

                prefix = esm.getFriendlyRaw().substr(0, 24);
                suffix = esm.getFriendlyRaw().substr(88);

                setNewFriendly(yampt::rec_type::CELL);

                if(to_convert == true)
                {
                    new_friendly.resize(64);
                    convertRecordContent(prefix + new_friendly + suffix);
                }
            }
        }

        if(esm.getRecordId() == "GAME")
        {
            esm.setUniqueTo("GMDT");
            esm.setFriendlyTo("GMDT");

            if(esm.getUniqueStatus() == true)
            {
                unique_key = esm.getUniqueRaw().substr(0, 64);
                eraseNullChars(unique_key);
                unique_key = "CELL" + yampt::sep[0] + unique_key;

                suffix = esm.getFriendlyRaw().substr(64);

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
