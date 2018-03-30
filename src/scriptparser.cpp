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

    while(std::getline(ss, line))
    {
        keyword_found = false;
        line = tools.eraseCarriageReturnChar(line);
        line_lc = line;

        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "messagebox", false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "choice", false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "say ", true);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "say,", true);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "addtopic", 0, yampt::rec_type::DIAL, false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "showmap", 0, yampt::rec_type::CELL, false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "centeroncell", 0, yampt::rec_type::CELL, false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "getpccell", 0, yampt::rec_type::CELL, true);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "aifollowcell", 1, yampt::rec_type::CELL, false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "aiescortcell", 1, yampt::rec_type::CELL, false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "placeitemcell", 1, yampt::rec_type::CELL, false);
        }

        if(keyword_found == false)
        {
            new_line = checkLine(line, line_lc, "positioncell", 4, yampt::rec_type::CELL, false);
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
std::string ScriptParser::checkLine(const std::string &line,
                                    const std::string &line_lc,
                                    const std::string &keyword,
                                    const bool &is_say)
{
    std::string new_line = line;
    size_t keyword_pos = line_lc.find(keyword);
    if(keyword_pos != std::string::npos &&
       line.rfind(";", keyword_pos) == std::string::npos)
    {
        new_line = convertLine(line);
        convertLineInCompiledScriptData(line, new_line, is_say);
        keyword_found = true;
    }
    return new_line;
}

//----------------------------------------------------------
std::string ScriptParser::checkLine(const std::string &line,
                                    const std::string &line_lc,
                                    const std::string &keyword,
                                    const int &pos_in_expr,
                                    const yampt::rec_type &text_type,
                                    const bool &is_getpccell)
{
    std::string new_line = line;
    std::pair<std::string, size_t> text;
    std::string new_text;
    size_t keyword_pos = line_lc.find(keyword);
    if(keyword_pos != std::string::npos &&
       line.rfind(";", keyword_pos) == std::string::npos)
    {
        text = extractText(line, keyword_pos, pos_in_expr);
        new_text = findText(text.first, text_type);
        new_line = convertText(line, text.first, text.second, new_text);
        convertInnerTextInCompiledScriptData(text.first, new_text, is_getpccell);
        keyword_found = true;
    }
    return new_line;
}

//----------------------------------------------------------
std::string ScriptParser::convertLine(const std::string &line)
{
    std::string new_line = line;
    auto search = merger->getDict(type).find(line);
    if(search != merger->getDict(type).end())
    {
        if(line != search->second)
        {
            new_line = search->second;
        }
    }
    return new_line;
}

//----------------------------------------------------------
std::string ScriptParser::findText(const std::string &text,
                                   const yampt::rec_type &text_type)
{
    // Fast search in map
    auto search = merger->getDict(text_type).find(text);
    if(search != merger->getDict(text_type).end())
    {
        return search->second;
    }
    else // Slow search case aware
    {
        for(auto &elem : merger->getDict(text_type))
        {
            if(tools.caseInsensitiveStringCmp(text, elem.first) == true)
            {
                return elem.second;
            }
        }
    }
    return text;
}

//----------------------------------------------------------
std::string ScriptParser::convertText(const std::string &line,
                                      const std::string &text,
                                      const size_t &text_pos,
                                      const std::string &new_text)
{
    std::string new_line = line;
    new_line.erase(text_pos, text.size());
    if(new_line.substr(text_pos - 1, 1) == "\"")
    {
        new_line.insert(text_pos, new_text);
    }
    else
    {
        new_line.insert(text_pos, "\"" + new_text + "\"");
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
void ScriptParser::convertInnerTextInCompiledScriptData(const std::string &text,
                                                        const std::string &new_text,
                                                        const bool is_getpccell)
{
    pos_in_compiled = compiled_data.find(text, pos_in_compiled);
    if(pos_in_compiled != std::string::npos)
    {
        pos_in_compiled -= 1;
        compiled_data.erase(pos_in_compiled, 1);
        compiled_data.insert(pos_in_compiled, tools.convertUIntToStringByteArray(new_text.size()).substr(0, 1));
        pos_in_compiled += 1;
        compiled_data.erase(pos_in_compiled, text.size());
        compiled_data.insert(pos_in_compiled, new_text);

        if(is_getpccell == true)
        {
            size_t size = new_text.size() + 12;
            pos_in_compiled -= 8;
            compiled_data.erase(pos_in_compiled, 1);
            compiled_data.insert(pos_in_compiled, tools.convertUIntToStringByteArray(size).substr(0, 1));
            pos_in_compiled += size;
        }
        else
        {
            pos_in_compiled += new_text.size();
        }
    }
}

//----------------------------------------------------------
std::pair<std::string, size_t> ScriptParser::extractText(const std::string &line,
                                                         const size_t keyword_pos,
                                                         const int pos_in_expression)
{
    size_t cur_pos = keyword_pos;
    std::string cur_text;

    std::smatch found;
    int ctr = -1;

    // Find begin of searched text
    cur_pos = line.find_first_of(" \t,\"", cur_pos);
    cur_pos = line.find_first_not_of(" \t,", cur_pos);
    if(cur_pos != std::string::npos)
    {
        cur_text = line.substr(cur_pos);
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
        cur_pos = found.position(1) + cur_pos;
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
