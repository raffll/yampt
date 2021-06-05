#include "scriptparser.hpp"

//----------------------------------------------------------
ScriptParser::ScriptParser(
    const Tools::RecType type,
    const DictMerger & merger,
    const std::string & script_name,
    const std::string & file_name,
    const std::string & old_script,
    const std::string & old_SCDT
)
    : type(type)
    , merger(&merger)
    , script_name(script_name)
    , file_name(file_name)
    , old_script(old_script)
    , old_SCDT(old_SCDT)
    , new_SCDT(old_SCDT)
{
    convertScript();
    trimLastNewLineChars();
}

//----------------------------------------------------------
void ScriptParser::convertScript()
{
    std::istringstream ss(old_script);
    bool is_end = false;

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
        error = false;

        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

        if (line_lc == "end" ||
            line_lc.size() > 3 && line_lc.substr(0, 4) == "end ")
        {
            is_end = true;
        }

        if (!is_end)
        {
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
                Tools::addLog("Error: unknown error in script parser!\r\n");
                Tools::addLog("Line: " + line + "\r\n");
                error = true;
            }
        }

        if (error)
            dumpError();

        new_script += new_line + "\r\n";
    }
}

//----------------------------------------------------------
void ScriptParser::convertLine(
    const std::string & keyword,
    const int pos_in_expression,
    const Tools::RecType text_type)
{
    pos = line_lc.find(keyword);

    if (pos == std::string::npos)
        return;

    if (line.size() == keyword.size())
        return;

    std::string s = "\\b" + keyword + "\\b";
    std::regex r(s, std::regex::optimize);
    std::smatch found;
    std::regex_search(line_lc, found, r);
    if (found.empty())
        return;

    if (line.rfind(";", pos) != std::string::npos)
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
    Tools::addLog("\r\n\r\n", true);
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
    std::regex r("\"(.*?)\"", std::regex::optimize);
    std::smatch found;
    std::regex_search(old_text, found, r);
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
    if (new_text.size() < 2)
    {
        Tools::addLog("Error: result is too short\r\n", true);
        error = true;
    }
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

    if (new_SCDT.empty())
    {
        Tools::addLog("Error: SCDT is empty\r\n", true);
        error = true;
        return;
    }

    pos_c = new_SCDT.find(old_text, pos_c);
    if (pos_c == std::string::npos)
    {
        Tools::addLog("Error: not found in SCDT\r\n", true);
        error = true;
        return;
    }

    size_t old_size = Tools::convertStringByteArrayToUInt(new_SCDT.substr(pos_c - 1, 1));

    /* wtf! Sometimes old text can be null terminated */
    while (old_size != old_text.size() && old_size != old_text.size() + 1)
    {
        Tools::addLog(
            "Warning: " +
            std::to_string(old_size) + " != " + std::to_string(old_text.size()) + " " +
            old_text + " false positive in " + script_name + "\r\n", true);
        error = true;

        pos_c += old_text.size();
        pos_c = new_SCDT.find(old_text, pos_c);

        if (pos_c == std::string::npos)
        {
            Tools::addLog("Error: not found in SCDT\r\n", true);
            error = true;
            return;
        }

        old_size = Tools::convertStringByteArrayToUInt(new_SCDT.substr(pos_c - 1, 1));
    }

    pos_c -= 1;
    new_SCDT.erase(pos_c, 1);
    new_SCDT.insert(pos_c, Tools::convertUIntToStringByteArray(new_text.size()).substr(0, 1));
    pos_c += 1;
    new_SCDT.erase(pos_c, old_text.size());
    new_SCDT.insert(pos_c, new_text);

    if (is_getpccell)
    {
        /* additional getpccell size byte determines
           how many bytes from that byte to the end of expression */
        size_t end_of_expr;
        size_t expr_size;

        if (new_SCDT.substr(pos_c + new_text.size(), 1) != " ")
        {
            /* if expression ends exactly when inner text ends */
            end_of_expr = pos_c + new_text.size();
            pos_c = new_SCDT.rfind('X', pos_c) - 2;
            expr_size = end_of_expr - pos_c;
        }
        else
        {
            /* if expression ends with equals or inequal signs */
            /* +5 because of equation " == 1" size */
            end_of_expr = pos_c + new_text.size() + 5;
            pos_c = new_SCDT.rfind('X', pos_c) - 2;
            expr_size = end_of_expr - pos_c - 1;
        }

        new_SCDT.erase(pos_c, 1);
        new_SCDT.insert(pos_c, Tools::convertUIntToStringByteArray(expr_size).substr(0, 1));
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

    if (keyword_pos == std::string::npos)
        return;

    if (line.rfind(";", keyword_pos) != std::string::npos)
        return;

    if (line.find("\"", keyword_pos) == std::string::npos)
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
    Tools::addLog("\r\n\r\n", true);
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

    if (new_SCDT.empty())
    {
        Tools::addLog("Error: SCDT is empty\r\n", true);
        error = true;
        return;
    }

    std::vector<std::string> splitted_line = splitLine(line);
    std::vector<std::string> splitted_new_line = splitLine(new_line);

    if (splitted_line.size() != splitted_new_line.size())
    {
        Tools::addLog("Error: incompatible messages\r\n", true);
        error = true;
        return;
    }

    for (size_t i = 0; i < splitted_line.size(); i++)
    {
        ReplaceVerticalLinesByNewLine(splitted_line[i]);
        ReplaceVerticalLinesByNewLine(splitted_new_line[i]);

        pos_c = new_SCDT.find(splitted_line[i], pos_c);
        if (pos_c == std::string::npos)
        {
            Tools::addLog("Error: message not found in SCDT\r\n", true);
            error = true;
            return;
        }

        if (splitted_line[i] == splitted_new_line[i])
        {
            pos_c += splitted_line[i].size();
            continue;
        }

        if (splitted_line[i] == " " || splitted_line[i] == "\t")
        {
            Tools::addLog("Error: message is one whitespace character\r\n", true);
            error = true;
            return;
        }

        if (i == 0)
        {
            pos_c -= 2;
            new_SCDT.erase(pos_c, 2);
            new_SCDT.insert(
                pos_c,
                Tools::convertUIntToStringByteArray(splitted_new_line[i].size()).substr(0, 2));
            pos_c += 2;
            new_SCDT.erase(pos_c, splitted_line[i].size());
            new_SCDT.insert(pos_c, splitted_new_line[i]);
            pos_c += splitted_new_line[i].size();
        }
        else
        {
            pos_c -= 1;
            new_SCDT.erase(pos_c, 1);
            new_SCDT.insert(
                pos_c,
                Tools::convertUIntToStringByteArray(splitted_new_line[i].size() + 1).substr(0, 1));
            pos_c += 1;
            new_SCDT.erase(pos_c, splitted_line[i].size());
            new_SCDT.insert(pos_c, splitted_new_line[i]);
            pos_c += splitted_new_line[i].size();
        }
    }
}

//----------------------------------------------------------
std::vector<std::string> ScriptParser::splitLine(
    const std::string & cur_line) const
{
    std::string cur_line_tr = cur_line.substr(keyword_pos);
    if (cur_line_tr.find(";") != std::string::npos)
    {
        cur_line_tr = cur_line_tr.substr(0, cur_line_tr.find(";"));
    }

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

    /* special case if say keyword */
    /* first parameter is sound file name, so we don't need it */
    if (keyword == "say" && splitted_line.size() > 0)
    {
        splitted_line.erase(splitted_line.begin());
    }

    return splitted_line;
}

//----------------------------------------------------------
void ScriptParser::trimLastNewLineChars()
{
    /* check if last 2 chars are newline and strip them if necessary */
    size_t last_nl_pos = old_script.rfind("\r\n");
    if (last_nl_pos != old_script.size() - 2 ||
        last_nl_pos == std::string::npos)
    {
        new_script.resize(new_script.size() - 2);
    }
}

//----------------------------------------------------------
void ScriptParser::dumpError()
{
    if (type == Tools::RecType::SCTX)
    {
        Tools::addLog("----------------------------------------------------------\r\n", true);
        Tools::addLog(Tools::replaceNonReadableCharsWithDot(old_SCDT), true);
        Tools::addLog("\r\n----------------------------------------------------------\r\n", true);
        Tools::addLog(Tools::replaceNonReadableCharsWithDot(new_SCDT), true);
    }
    Tools::addLog("\r\n----------------------------------------------------------\r\n", true);
    Tools::addLog(old_script, true);
    Tools::addLog("\r\n----------------------------------------------------------\r\n", true);
}

//----------------------------------------------------------
void ScriptParser::ReplaceVerticalLinesByNewLine(std::string & message)
{
    while (message.find("|") != std::string::npos)
    {
        message.replace(message.find("|"), 1, "\x0A");
    }
}
