#include "scriptparser.hpp"

//----------------------------------------------------------
ScriptParser::ScriptParser()
{

}

//----------------------------------------------------------
ScriptParser::ScriptParser(const yampt::rec_type type,
                           DictMerger &merger,
                           const std::string &friendly_text)
    : type(type),
      merger(&merger),
      friendly_text(friendly_text),
      pos_in_compiled(0)
{

}

//----------------------------------------------------------
ScriptParser::ScriptParser(const yampt::rec_type type,
                           DictMerger &merger,
                           const std::string &friendly_text,
                           const std::string &compiled_data)
    : type(type),
      merger(&merger),
      friendly_text(friendly_text),
      compiled_data(compiled_data),
      pos_in_compiled(0)
{

}

//----------------------------------------------------------
void ScriptParser::convertScript()
{
    counter_all++;
    new_friendly.erase();
    std::istringstream ss(friendly_text);
    std::string line;
    std::string line_lc;
    std::string new_line;
    size_t keyword_pos;

            std::cout << " duap";

    while(std::getline(ss, line))
    {
        to_convert = false;
        line = tools.eraseCarriageReturnChar(line);
        line_lc = line;
        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("messagebox");
            new_line = convertLine(line, keyword_pos, false);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("choice");
            new_line = convertLine(line, keyword_pos, false);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("say ");  // ugly keyword
            new_line = convertLine(line, keyword_pos, true);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("say,");
            new_line = convertLine(line, keyword_pos, true);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("addtopic");
            new_line = convertInnerText(line, keyword_pos, 0, false, yampt::rec_type::DIAL);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("showmap");
            new_line = convertInnerText(line, keyword_pos, 0, false, yampt::rec_type::CELL);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("centeroncell");
            new_line = convertInnerText(line, keyword_pos, 0, false, yampt::rec_type::CELL);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("getpccell");
            new_line = convertInnerText(line, keyword_pos, 0, true, yampt::rec_type::CELL);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("aifollowcell");
            new_line = convertInnerText(line, keyword_pos, 1, false, yampt::rec_type::CELL);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("aiescortcell)");
            new_line = convertInnerText(line, keyword_pos, 1, false, yampt::rec_type::CELL);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("placeitemcell");
            new_line = convertInnerText(line, keyword_pos, 1, false, yampt::rec_type::CELL);
        }

        if(to_convert == false)
        {
            keyword_pos = line_lc.find("positioncell");
            new_line = convertInnerText(line, keyword_pos, 4, false, yampt::rec_type::CELL);
        }

        new_friendly += new_line + "\r\n";

    }

    // Check if last 2 char are new line
    size_t last_nl_pos = friendly_text.rfind("\r\n");
    if(last_nl_pos != friendly_text.size() - 2 || last_nl_pos == std::string::npos)
    {
        new_friendly.resize(new_friendly.size() - 2);
    }
}

//----------------------------------------------------------
std::string ScriptParser::convertLine(const std::string &line,
                                      const size_t keyword_pos,
                                      const bool is_say)
{
    std::string new_line = line;
    if(keyword_pos != std::string::npos &&
       line.rfind(";", keyword_pos) == std::string::npos)
    {
        auto search = merger->getDict(type).find(line);
        if(search != merger->getDict(type).end())
        {
            if(line != search->second)
            {
                to_convert = true;
                new_line = search->second;
                convertLineInCompiledScriptData(line, new_line, is_say);
            }
        }
    }
    return new_line;
}

//----------------------------------------------------------
std::string ScriptParser::convertInnerText(const std::string &line,
                                           const size_t keyword_pos,
                                           const int pos_in_expression,
                                           const bool is_getpccell,
                                           const yampt::rec_type inner_type)
{
    std::string new_line = line;
    std::pair<std::string, size_t> temp = extractText(line, keyword_pos, pos_in_expression);
    std::string inner_text = temp.first;
    size_t inner_pos = temp.second;

    if(keyword_pos != std::string::npos &&
       line.rfind(";", keyword_pos) == std::string::npos)
    {
        // Fast search in map
        auto search = merger->getDict(inner_type).find(inner_text);
        if(search != merger->getDict(inner_type).end())
        {
            try
            {
                new_line.erase(inner_pos, inner_text.size());
                convertInnerTextInCompiledScriptData(inner_text, search->second, is_getpccell);

                if(new_line.substr(inner_pos - 1, 1) == "\"")
                {
                    new_line.insert(inner_pos, search->second);
                }
                else
                {
                    new_line.insert(inner_pos, "\"" + search->second + "\"");
                }
                to_convert = true;
                return new_line;
            }
            catch(std::exception const& e)
            {
                std::cout << "--> Error in function convertInnerText()" << std::endl;
                std::cout << line << std::endl;
                std::cout << new_line << std::endl;
                std::cout << "--> Exception: " << e.what() << std::endl;
            }
        }
        else // Slow search case aware
        {
            for(auto &elem : merger->getDict(inner_type))
            {
                if(tools.caseInsensitiveStringCmp(inner_text, elem.first) == true)
                {
                    try
                    {
                        new_line.erase(inner_pos, inner_text.size());
                        convertInnerTextInCompiledScriptData(inner_text, elem.second, is_getpccell);

                        if(new_line.substr(inner_pos - 1, 1) == "\"")
                        {
                            new_line.insert(inner_pos, elem.second);
                        }
                        else
                        {
                            new_line.insert(inner_pos, "\"" + elem.second + "\"");
                        }
                        to_convert = true;
                        return new_line;
                    }
                    catch(std::exception const& e)
                    {
                        std::cout << "--> Error in function convertInnerText()" << std::endl;
                        std::cout << line << std::endl;
                        std::cout << new_line << std::endl;
                        std::cout << "--> Exception: " << e.what() << std::endl;
                    }
                }
            }
        }
    }
    return new_line;
}

//----------------------------------------------------------
void ScriptParser::convertLineInCompiledScriptData(const std::string &line,
                                                   const std::string &new_line,
                                                   const bool is_say)
{
    std::vector<std::string> splitted_line = splitLine(line, is_say);
    std::vector<std::string> splitted_new_line = splitLine(new_line, is_say);

    if(splitted_line.size() == splitted_new_line.size())
    {
        for(size_t i = 0; i < splitted_line.size(); i++)
        {
            pos_in_compiled = compiled_data.find(splitted_line[i], pos_in_compiled);
            if(pos_in_compiled != std::string::npos)
            {
                if(i == 0)
                {
                    pos_in_compiled -= 2;
                    compiled_data.erase(pos_in_compiled, 2);
                    compiled_data.insert(pos_in_compiled, tools.convertUIntToStringByteArray(splitted_new_line[i].size()).substr(0, 2));
                    pos_in_compiled += 2;
                    compiled_data.erase(pos_in_compiled, splitted_line[i].size());
                    compiled_data.insert(pos_in_compiled, splitted_new_line[i]);
                    pos_in_compiled += splitted_new_line[i].size();
                }
                else
                {
                    pos_in_compiled -= 1;
                    compiled_data.erase(pos_in_compiled, 1);
                    compiled_data.insert(pos_in_compiled, tools.convertUIntToStringByteArray(splitted_new_line[i].size() + 1).substr(0, 1));
                    pos_in_compiled += 1;
                    compiled_data.erase(pos_in_compiled, splitted_line[i].size());
                    compiled_data.insert(pos_in_compiled, splitted_new_line[i]);
                    pos_in_compiled += splitted_new_line[i].size();
                }
            }
        }
    }
}

//----------------------------------------------------------
void ScriptParser::convertInnerTextInCompiledScriptData(const std::string &inner_text,
                                                        const std::string &new_inner_text,
                                                        const bool is_getpccell)
{
    pos_in_compiled = compiled_data.find(inner_text, pos_in_compiled);
    if(pos_in_compiled != std::string::npos)
    {
        pos_in_compiled -= 1;
        compiled_data.erase(pos_in_compiled, 1);
        compiled_data.insert(pos_in_compiled, tools.convertUIntToStringByteArray(new_inner_text.size()).substr(0, 1));
        pos_in_compiled += 1;
        compiled_data.erase(pos_in_compiled, inner_text.size());
        compiled_data.insert(pos_in_compiled, new_inner_text);

        if(is_getpccell == true)
        {
            size_t size = new_inner_text.size() + 12;
            pos_in_compiled -= 8;
            compiled_data.erase(pos_in_compiled, 1);
            compiled_data.insert(pos_in_compiled, tools.convertUIntToStringByteArray(size).substr(0, 1));
            pos_in_compiled += size;
        }
        else
        {
            pos_in_compiled += new_inner_text.size();
        }
    }
}

//----------------------------------------------------------
std::pair<std::string, size_t> ScriptParser::extractText(const std::string &line,
                                                         const size_t keyword_pos,
                                                         const int pos_in_expression)
{
    size_t cur_pos = keyword_pos;
    std::string cur_text = line.substr(keyword_pos);

    std::cout << cur_text << std::endl;

    std::smatch found;
    int ctr = -1;

    // Find begin of searched text
    cur_pos = cur_text.find_first_of(" \t,\"", cur_pos);
    cur_pos = cur_text.find_first_not_of(" \t,", cur_pos);
    if(cur_pos != std::string::npos)
    {
        cur_text = cur_text.substr(cur_pos);
        std::cout << cur_text << std::endl;
    }

    if(pos_in_expression == 0)
    {
        // Not all searched texts are in quotes
        // Find end of searched text if it is in GETPCCELL
        if(cur_text.find("=") != std::string::npos)
        {
            cur_text.erase(cur_text.find("="));
        }

        // Find end of searched text if it is in ADDTOPIC, SHOWMAP, CENTERONCELL
        if(cur_text.find_last_not_of(" \t") != std::string::npos)
        {
            cur_text.erase(cur_text.find_last_not_of(" \t") + 1);
        }
    }
    else
    {
        // Usually all searched texts are in quotes, but other variables don't
        std::regex r1("(([\\w\\.]+)|(\".*?\"))");
        std::sregex_iterator next(cur_text.begin(), cur_text.end(), r1);
        std::sregex_iterator end;
        while(next != end && ctr != pos_in_expression)
        {
            found = *next;
            next++;
            ctr++;
        }
        cur_text = found[1].str();
        cur_pos = found.position(1);
    }

    // Strip quotes if exist
    std::regex r2("\"(.*?)\"");
    std::regex_search(cur_text, found, r2);
    if(!found.empty())
    {
        cur_text = found[1].str();
        cur_pos += 1;
    }

    return std::make_pair(cur_text, cur_pos);
}

//----------------------------------------------------------
std::vector<std::string> ScriptParser::splitLine(const std::string &line,
                                                 const bool is_say)
{
    std::vector<std::string> splitted_line;
    std::smatch found;
    std::regex re("\"(.*?)\"");
    std::sregex_iterator next(line.begin(), line.end(), re);
    std::sregex_iterator end;
    while(next != end)
    {
        found = *next;
        splitted_line.push_back(found[1].str());
        next++;
    }

    // Special case if SAY keyword
    if(is_say == true && splitted_line.size() > 0)
    {
        splitted_line.erase(splitted_line.begin());
    }

    return splitted_line;
}
