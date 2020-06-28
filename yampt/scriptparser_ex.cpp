#include "scriptparser_ex.hpp"

//----------------------------------------------------------
ScriptParser::ScriptParser(
    const Tools::RecType type,
    const DictMerger & merger,
    const std::string & script_name,
    const std::string & file_name,
    const std::string & val_text,
    const std::string & new_compiled
)
    : type(type)
    , merger(&merger)
    , script_name(script_name)
    , file_name(file_name)
    , val_text(val_text)
    , new_compiled(new_compiled)
{
    convertScript();
}

//----------------------------------------------------------
void ScriptParser::convertScript()
{
    std::istringstream ss(val_text);

    while (std::getline(ss, line))
    {
        is_done = false;
        line = Tools::trimCR(line);
        line_lc = line;
        new_line = line;
        new_text.erase();
        pos = 0;
        keyword_pos = 0;
        keyword.erase();

        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

        try
        {
            if (!is_done)
                convertLine("addtopic", 0, Tools::RecType::DIAL);

            if (!is_done)
                convertLine("showmap", 0, Tools::RecType::CELL);

            if (!is_done)
                convertLine("centeroncell", 0, Tools::RecType::CELL);

            if (!is_done)
                convertLine("getpccell", 0, Tools::RecType::CELL);

            if (!is_done)
                convertLine("aifollowcell", 1, Tools::RecType::CELL);

            if (!is_done)
                convertLine("aiescortcell", 1, Tools::RecType::CELL);

            if (!is_done)
                convertLine("placeitemcell", 1, Tools::RecType::CELL);

            if (!is_done)
                convertLine("positioncell", 4, Tools::RecType::CELL);

            if (!is_done)
                convertLine();
        }
        catch (...)
        {
            Tools::addLog("Error: unknown in script parser!\r\n");
            Tools::addLog("Line: " + line + "\r\n");
        }

        new_friendly += new_line + "\r\n";
    }

    trimLastNewLineChars();
}

//----------------------------------------------------------
void ScriptParser::convertLine(
    const std::string & keyword,
    const int pos_in_expression,
    const Tools::RecType text_type)
{
    pos = line_lc.find(keyword);

    if (pos == std::string::npos ||
        line.rfind(";", pos) != std::string::npos)
        return;

    pos = line.find_first_of(" \t,\"", pos);
    pos = line.find_first_not_of(" \t,", pos);
    if (pos == std::string::npos)
        return;

    trimLine();
    extractText(pos_in_expression);
    removeQuotes();
    findNewText(text_type);
    insertNewText();

    const auto is_getpccell = keyword == "getpccell" ? true : false;
    convertTextInCompiled(is_getpccell);

    is_done = true;
}

//----------------------------------------------------------
void ScriptParser::trimLine()
{
    Tools::addLog("---\r\n", true);
    Tools::addLog(file_name + "\r\n", true);
    Tools::addLog(script_name + "\r\n", true);
    Tools::addLog("<<< " + line + "\r\n", true);

    old_text = line.substr(pos);

    Tools::addLog("1: " + old_text + "\r\n", true);
}

//----------------------------------------------------------
void ScriptParser::extractText(const int pos_in_expression)
{
    std::regex r1("([\\w\\.\\-\\xD1]+|\".*?\")", std::regex::optimize);
    std::sregex_iterator next(old_text.begin(), old_text.end(), r1);
    std::sregex_iterator end;
    std::smatch found;

    int ctr = -1;
    while (next != end && ctr != pos_in_expression)
    {
        found = *next;
        next++;
        ctr++;
    }

    old_text = found[1].str();
    pos += found.position(1);

    Tools::addLog("2: " + old_text + "\r\n", true);
}

//----------------------------------------------------------
void ScriptParser::removeQuotes()
{
    std::regex r2("\"(.*?)\"", std::regex::optimize);
    std::smatch found;
    std::regex_search(old_text, found, r2);
    if (!found.empty())
    {
        old_text = found[1].str();
        pos += 1;
    }

    Tools::addLog("3: " + old_text + "\r\n", true);
}

//----------------------------------------------------------
void ScriptParser::findNewText(const Tools::RecType text_type)
{
    new_text = old_text;
    auto search = merger->getDict().at(text_type).find(old_text);
    if (search != merger->getDict().at(text_type).end())
    {
        new_text = search->second;
    }
    else
    {
        for (const auto & elem : merger->getDict().at(text_type))
        {
            if (Tools::caseInsensitiveStringCmp(old_text, elem.first))
            {
                new_text = elem.second;
                break;
            }
        }
    }

    Tools::addLog("4: " + new_text + "\r\n", true);
}

//----------------------------------------------------------
void ScriptParser::insertNewText()
{
    new_line.erase(pos, old_text.size());
    new_line.insert(pos, new_text);

    Tools::addLog(">>> " + new_line + "\r\n", true);
}

//----------------------------------------------------------
void ScriptParser::convertTextInCompiled(const bool is_getpccell)
{
    if (type != Tools::RecType::SCTX)
        return;

    if (new_compiled.empty())
    {
        Tools::addLog("Error: SCDT is empty\r\n", true);
        return;
    }

    pos_c = new_compiled.find(old_text, pos_c);
    if (pos_c == std::string::npos)
    {
        Tools::addLog("Error: not found in SCDT\r\n", true);
        return;
    }
    size_t old_size = Tools::convertStringByteArrayToUInt(new_compiled.substr(pos_c - 1, 1));

    // WTF! Sometimes old text can be null terminated
    while (old_size != old_text.size() && old_size != old_text.size() + 1)
    {
        Tools::addLog(
            "Warning: " +
            std::to_string(old_size) + " != " + std::to_string(old_text.size()) + " " +
            old_text + " false positive in " + script_name + "\r\n", true);
        pos_c += old_text.size();

        pos_c = new_compiled.find(old_text, pos_c);
        if (pos_c == std::string::npos)
        {
            Tools::addLog("Error: not found in SCDT\r\n", true);
            return;
        }
        old_size = Tools::convertStringByteArrayToUInt(new_compiled.substr(pos_c - 1, 1));
    }

    pos_c -= 1;
    new_compiled.erase(pos_c, 1);
    new_compiled.insert(pos_c, Tools::convertUIntToStringByteArray(new_text.size()).substr(0, 1));
    pos_c += 1;
    new_compiled.erase(pos_c, old_text.size());
    new_compiled.insert(pos_c, new_text);

    if (is_getpccell)
    {
        // Additional getpccell size byte determines
        // how many bytes from that byte to the end of expression
        size_t end_of_expr;
        size_t expr_size;

        if (new_compiled.substr(pos_c + new_text.size(), 1) != " ")
        {
            // If expression ends exactly when inner text ends
            end_of_expr = pos_c + new_text.size();
            pos_c = new_compiled.rfind('X', pos_c) - 2;
            expr_size = end_of_expr - pos_c;
        }
        else
        {
            // If expression ends with equals or inequal signs
            end_of_expr = pos_c + new_text.size() + 5; // + 5 because of equation " == 1" size
            pos_c = new_compiled.rfind('X', pos_c) - 2;
            expr_size = end_of_expr - pos_c - 1;
        }

        new_compiled.erase(pos_c, 1);
        new_compiled.insert(pos_c, Tools::convertUIntToStringByteArray(expr_size).substr(0, 1));
        pos_c += expr_size;
    }
    else
    {
        pos_c += new_text.size();
    }
}

//----------------------------------------------------------
void ScriptParser::convertLine()
{
    findKeyword();

    if (keyword_pos == std::string::npos ||
        line.rfind(";", keyword_pos) != std::string::npos ||
        line.find("\"", keyword_pos) == std::string::npos)
        return;

    findNewMessage();
    convertMessageInCompiled();

    is_done = true;
}

//----------------------------------------------------------
void ScriptParser::findKeyword()
{
    std::map<size_t, std::string> keyword_pos_coll;
    for (const auto & keyword : Tools::keywords)
    {
        keyword_pos = line_lc.find(keyword);
        keyword_pos_coll.insert({ keyword_pos, keyword });
    }

    keyword_pos = keyword_pos_coll.begin()->first;
    keyword = keyword_pos_coll.begin()->second;
}

//----------------------------------------------------------
void ScriptParser::findNewMessage()
{
    Tools::addLog("---\r\n", true);
    Tools::addLog(file_name + "\r\n", true);
    Tools::addLog(script_name + "\r\n", true);
    Tools::addLog("<<< " + line + "\r\n", true);

    auto search = merger->getDict().at(type).find(script_name + Tools::sep[0] + line);
    if (search != merger->getDict().at(type).end())
    {
        if (line != search->second.substr(script_name.size() + 1))
        {
            new_line = search->second.substr(script_name.size() + 1);
        }
    }

    Tools::addLog(">>> " + new_line + "\r\n", true);
}

//----------------------------------------------------------
void ScriptParser::convertMessageInCompiled()
{
    if (type != Tools::RecType::SCTX)
        return;

    if (new_compiled.empty())
    {
        Tools::addLog("Error: SCDT is empty\r\n", true);
        return;
    }

    std::vector<std::string> splitted_line = splitLine(line);
    std::vector<std::string> splitted_new_line = splitLine(new_line);

    if (splitted_line.size() != splitted_new_line.size())
    {
        Tools::addLog("Error: incompatible messages\r\n", true);
        return;
    }

    for (size_t i = 0; i < splitted_line.size(); i++)
    {
        pos_c = new_compiled.find(splitted_line[i], pos_c);
        if (pos_c == std::string::npos)
        {
            Tools::addLog("Error: message not found in SCDT\r\n", true);
            return;
        }

        if (splitted_line[i] == splitted_new_line[i])
        {
            Tools::addLog("Warning: message skipped in SCDT\r\n", true);
            pos_c += splitted_line[i].size();
            continue;
        }

        if (splitted_line[i] == " " || splitted_line[i] == "\t")
        {
            Tools::addLog("Error: message is one whitespace character\r\n", true);
            return;
        }

        if (i == 0)
        {
            pos_c -= 2;
            new_compiled.erase(pos_c, 2);
            new_compiled.insert(
                pos_c,
                Tools::convertUIntToStringByteArray(splitted_new_line[i].size()).substr(0, 2));
            pos_c += 2;
            new_compiled.erase(pos_c, splitted_line[i].size());
            new_compiled.insert(pos_c, splitted_new_line[i]);
            pos_c += splitted_new_line[i].size();
        }
        else
        {
            pos_c -= 1;
            new_compiled.erase(pos_c, 1);
            new_compiled.insert(
                pos_c,
                Tools::convertUIntToStringByteArray(splitted_new_line[i].size() + 1).substr(0, 1));
            pos_c += 1;
            new_compiled.erase(pos_c, splitted_line[i].size());
            new_compiled.insert(pos_c, splitted_new_line[i]);
            pos_c += splitted_new_line[i].size();
        }
    }
}

//----------------------------------------------------------
std::vector<std::string> ScriptParser::splitLine(
    const std::string cur_line) const
{
    const std::string cur_line_tr = cur_line.substr(keyword_pos);
    std::vector<std::string> splitted_line;
    std::regex re("\"(.*?)\"", std::regex::optimize);
    std::sregex_iterator next(cur_line_tr.begin(), cur_line_tr.end(), re);
    std::sregex_iterator end;
    std::smatch found;
    while (next != end)
    {
        found = *next;
        splitted_line.push_back(found[1].str());
        next++;
    }

    // Special case if say keyword
    // First parameter is sound file name, so we don't need it
    if (keyword == "say" && splitted_line.size() > 0)
    {
        splitted_line.erase(splitted_line.begin());
    }

    return splitted_line;
}

//----------------------------------------------------------
void ScriptParser::trimLastNewLineChars()
{
    // Check if last 2 chars are newline and strip them if necessary
    size_t last_nl_pos = val_text.rfind("\r\n");
    if (last_nl_pos != val_text.size() - 2 ||
        last_nl_pos == std::string::npos)
    {
        new_friendly.resize(new_friendly.size() - 2);
    }
}