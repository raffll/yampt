#include "scriptparser.hpp"

//----------------------------------------------------------
ScriptParser::ScriptParser()
{

}

//----------------------------------------------------------
ScriptParser::ScriptParser(
    const Tools::RecType type,
    const DictMerger & merger,
    const std::string & prefix,
    const std::string & friendly_text,
    const std::string & compiled_data
)
    : type(type)
    , merger(&merger)
    , prefix(prefix)
    , friendly_text(friendly_text)
    , compiled_data(compiled_data)
    , pos_in_compiled(0)
{
    convertScript();
    stripLastNewLineChars();
}

//----------------------------------------------------------
void ScriptParser::convertScript()
{
    std::istringstream ss(friendly_text);
    std::string line;
    std::string line_lc;
    std::string new_line;

    while (std::getline(ss, line))
    {
        is_done = false;
        line = Tools::trimCR(line);
        line_lc = line;
        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "addtopic", 0, Tools::RecType::DIAL, false);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "showmap", 0, Tools::RecType::CELL, false);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "centeroncell", 0, Tools::RecType::CELL, false);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "getpccell", 0, Tools::RecType::CELL, true);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "aifollowcell", 1, Tools::RecType::CELL, false);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "aiescortcell", 1, Tools::RecType::CELL, false);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "placeitemcell", 1, Tools::RecType::CELL, false);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc, "positioncell", 4, Tools::RecType::CELL, false);
        }
        if (is_done == false)
        {
            new_line = checkLine(line, line_lc);
        }
        new_friendly += new_line + "\r\n";
    }
}

//----------------------------------------------------------
void ScriptParser::stripLastNewLineChars()
{
    // Check if last 2 chars are newline and strip them if necessary
    size_t last_nl_pos = friendly_text.rfind("\r\n");
    if (last_nl_pos != friendly_text.size() - 2 ||
        last_nl_pos == std::string::npos)
    {
        new_friendly.resize(new_friendly.size() - 2);
    }
}

//----------------------------------------------------------
std::string ScriptParser::checkLine(
    const std::string & line,
    const std::string & line_lc)
{
    std::string new_line = line;
    std::map<size_t, std::string> keyword_pos_coll;
    std::string keyword;
    size_t keyword_pos;

    // Create keyword collection from first to last occurrence in line
    for (size_t i = 0; i < Tools::keywords.size(); ++i)
    {
        keyword_pos = line_lc.find(Tools::keywords[i]);
        keyword_pos_coll.insert({ keyword_pos, Tools::keywords[i] });
    }

    keyword = keyword_pos_coll.begin()->second;
    keyword_pos = keyword_pos_coll.begin()->first;

    if (keyword_pos != std::string::npos &&
        line.rfind(";", keyword_pos) == std::string::npos &&
        line.find("\"", keyword_pos) != std::string::npos) // Convert only if message line is valid
    {
        Tools::addLog("---\r\n");
        Tools::addLog("Old line: " + line + "\r\n");
        new_line = convertLine(line);
        Tools::addLog("New line: " + new_line + "\r\n");
        if (!compiled_data.empty())
        {
            if (keyword == "say " ||
                keyword == "say,")
            {
                convertLineInCompiledScriptData(line, new_line, keyword_pos, true);
            }
            else if (keyword == "messagebox" ||
                     keyword == "choice")
            {
                convertLineInCompiledScriptData(line, new_line, keyword_pos, false);
            }
        }
        is_done = true;
    }

    return new_line;
}

//----------------------------------------------------------
std::string ScriptParser::checkLine(
    const std::string & line,
    const std::string & line_lc,
    const std::string & keyword,
    const int pos_in_expression,
    const Tools::RecType text_type,
    const bool is_getpccell)
{
    std::string new_line = line;
    std::pair<std::string, size_t> extracted;
    std::string new_text;
    size_t keyword_pos = line_lc.find(keyword);

    if (keyword_pos != std::string::npos &&
        line.rfind(";", keyword_pos) == std::string::npos)
    {
        Tools::addLog("---\r\n");
        Tools::addLog("Old line: " + line + "\r\n");
        Tools::addLog("Extraction:\r\n");
        extracted = extractInnerTextFromLine(line, keyword_pos, pos_in_expression);

        // Convert only if extracted text is valid
        if (extracted.second != std::string::npos)
        {
            new_text = findInnerTextInDict(extracted.first, text_type);
            Tools::addLog("Found: " + new_text + "\r\n");
            new_line = convertInnerTextInLine(line, extracted.first, extracted.second, new_text);
            Tools::addLog("New line: " + new_line + "\r\n");
            if (!compiled_data.empty())
            {
                convertInnerTextInCompiledScriptData(extracted.first, new_text, is_getpccell);
            }
        }
        is_done = true;
    }
    return new_line;
}

//----------------------------------------------------------
std::string ScriptParser::convertLine(const std::string & line)
{
    std::string new_line = line;
    auto search = merger->getDict(type).find(prefix + line);
    if (search != merger->getDict(type).end())
    {
        if (line != search->second.substr(prefix.size()))
        {
            new_line = search->second.substr(prefix.size());
        }
    }
    return new_line;
}

//----------------------------------------------------------
std::string ScriptParser::findInnerTextInDict(
    const std::string & text,
    const Tools::RecType text_type)
{
    // Fast search in map
    auto search = merger->getDict(text_type).find(text);
    if (search != merger->getDict(text_type).end())
    {
        return search->second;
    }
    else // Slow search case aware
    {
        for (auto & elem : merger->getDict(text_type))
        {
            if (Tools::caseInsensitiveStringCmp(text, elem.first) == true)
            {
                return elem.second;
            }
        }
    }
    return text;
}

//----------------------------------------------------------
std::string ScriptParser::convertInnerTextInLine(
    const std::string & line,
    const std::string & text,
    const size_t text_pos,
    const std::string & new_text)
{
    std::string new_line = line;
    new_line.erase(text_pos, text.size());
    if (new_line.substr(text_pos - 1, 1) == "\"")
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
void ScriptParser::convertLineInCompiledScriptData(
    const std::string & line,
    const std::string & new_line,
    const size_t keyword_pos,
    const bool is_say)
{
    std::vector<std::string> splitted_line = splitLine(line, keyword_pos, is_say);
    std::vector<std::string> splitted_new_line = splitLine(new_line, keyword_pos, is_say);

    if (splitted_line.size() == splitted_new_line.size())
    {
        for (size_t i = 0; i < splitted_line.size(); i++)
        {
            pos_in_compiled = compiled_data.find(splitted_line[i], pos_in_compiled);
            if (pos_in_compiled != std::string::npos)
            {
                if (i == 0)
                {
                    pos_in_compiled -= 2;
                    compiled_data.erase(pos_in_compiled, 2);
                    compiled_data.insert(pos_in_compiled, Tools::convertUIntToStringByteArray(splitted_new_line[i].size()).substr(0, 2));
                    pos_in_compiled += 2;
                    compiled_data.erase(pos_in_compiled, splitted_line[i].size());
                    compiled_data.insert(pos_in_compiled, splitted_new_line[i]);
                    pos_in_compiled += splitted_new_line[i].size();
                }
                else
                {
                    pos_in_compiled -= 1;
                    compiled_data.erase(pos_in_compiled, 1);
                    compiled_data.insert(pos_in_compiled, Tools::convertUIntToStringByteArray(splitted_new_line[i].size() + 1).substr(0, 1));
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
void ScriptParser::convertInnerTextInCompiledScriptData(
    const std::string & text,
    const std::string & new_text,
    const bool is_getpccell)
{
    pos_in_compiled = compiled_data.find(text, pos_in_compiled);
    if (pos_in_compiled != std::string::npos)
    {
        pos_in_compiled -= 1;
        compiled_data.erase(pos_in_compiled, 1);
        compiled_data.insert(pos_in_compiled, Tools::convertUIntToStringByteArray(new_text.size()).substr(0, 1));
        pos_in_compiled += 1;
        compiled_data.erase(pos_in_compiled, text.size());
        compiled_data.insert(pos_in_compiled, new_text);

        if (is_getpccell == true)
        {
            // Additional GETPCCELL size byte determines
            // how many bytes from that byte to the end of expression
            size_t end_of_expr;
            size_t expr_size;

            if (compiled_data.substr(pos_in_compiled + new_text.size(), 1) != " ")
            {
                // If expression ends exactly when inner text ends
                end_of_expr = pos_in_compiled + new_text.size();
                pos_in_compiled = compiled_data.rfind('X', pos_in_compiled) - 2;
                expr_size = end_of_expr - pos_in_compiled;
            }
            else
            {
                // If expression ends with equals or inequal signs
                end_of_expr = pos_in_compiled + new_text.size() + 5; // +5 because of " == 1" size
                pos_in_compiled = compiled_data.rfind('X', pos_in_compiled) - 2;
                expr_size = end_of_expr - pos_in_compiled - 1;
            }

            compiled_data.erase(pos_in_compiled, 1);
            compiled_data.insert(pos_in_compiled, Tools::convertUIntToStringByteArray(expr_size).substr(0, 1));
            pos_in_compiled += expr_size;
        }
        else
        {
            pos_in_compiled += new_text.size();
        }
    }
}

//----------------------------------------------------------
std::pair<std::string, size_t> ScriptParser::extractInnerTextFromLine(
    const std::string & line,
    const size_t keyword_pos,
    const int pos_in_expression)
{
    size_t cur_pos = keyword_pos;
    std::string cur_text;

    std::smatch found;
    int ctr = -1;

    // Find begin of searched inner text
    cur_pos = line.find_first_of(" \t,\"", cur_pos);
    cur_pos = line.find_first_not_of(" \t,", cur_pos);
    if (cur_pos != std::string::npos)
    {
        cur_text = line.substr(cur_pos);
    }
    else
    {
        // If begin of searched text not found
        // or keyword was found inside other string
        // e.g. name of the script
        cur_text = "Inner text not found!";
    }

    Tools::addLog("\tStep 1: " + cur_text + "\r\n");

    if (pos_in_expression == 0)
    {
        // Find end of searched text if keyword is not in quotes
        // Strip whitespaces
        if (cur_text.find_last_not_of(" \t") != std::string::npos)
        {
            cur_text.erase(cur_text.find_last_not_of(" \t") + 1);
        }
    }
    else
    {
        // Usually all searched texts are in quotes, but other variables don't
        std::regex r1("(([\\w\\.]+)|(\".*?\"))", std::regex::optimize);
        std::sregex_iterator next(cur_text.begin(), cur_text.end(), r1);
        std::sregex_iterator end;
        while (next != end && ctr != pos_in_expression)
        {
            found = *next;
            next++;
            ctr++;
        }
        cur_text = found[1].str();
        cur_pos = found.position(1) + cur_pos;
    }

    Tools::addLog("\tStep 2: " + cur_text + "\r\n");

    // Strip quotes if exist
    std::regex r2("\"(.*?)\"", std::regex::optimize);
    std::regex_search(cur_text, found, r2);
    if (!found.empty())
    {
        cur_text = found[1].str();
        cur_pos += 1;
    }

    Tools::addLog("\tStep 3: " + cur_text + "\r\n");

    return std::make_pair(cur_text, cur_pos);
}

//----------------------------------------------------------
std::vector<std::string> ScriptParser::splitLine(
    const std::string & line,
    const size_t keyword_pos,
    const bool is_say)
{
    // We only need parameters after keyword
    std::string cur_line = line.substr(keyword_pos);
    std::vector<std::string> splitted_line;
    std::smatch found;
    std::regex re("\"(.*?)\"", std::regex::optimize);
    std::sregex_iterator next(cur_line.begin(), cur_line.end(), re);
    std::sregex_iterator end;
    while (next != end)
    {
        found = *next;
        splitted_line.push_back(found[1].str());
        next++;
    }

    // Special case if SAY keyword
    // First parameter is sound file name, so we don't need it
    if (is_say == true && splitted_line.size() > 0)
    {
        splitted_line.erase(splitted_line.begin());
    }

    return splitted_line;
}
