#include "scriptparser.hpp"

//----------------------------------------------------------
ScriptParser::ScriptParser(
    const Tools::RecType type,
    const DictMerger & merger,
    const std::string & prefix,
    const std::string & friendly_text,
    const std::string & new_compiled
)
    : merger(&merger)
    //, prefix(prefix)
    //, friendly_text(friendly_text)
    , new_compiled(new_compiled)
    //, pos_in_compiled(0)
{
    convertScript(friendly_text);
}

//----------------------------------------------------------
void ScriptParser::convertScript(const std::string & friendly_text)
{
    std::istringstream ss(friendly_text);

    while (std::getline(ss, line))
    {
        is_done = false;
        line = Tools::trimCR(line);
        line_lc = line;
        new_line = line;
        pos = 0;

        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

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


        new_friendly += new_line;// +"\r\n";
    }
}

//----------------------------------------------------------
void ScriptParser::convertLine(
    const std::string & keyword,
    const int pos_in_expression,
    const Tools::RecType text_type)
{
    {
        pos = line_lc.find(keyword);

        if (pos == std::string::npos ||
            line.rfind(";", pos) != std::string::npos)
            return;

        pos = line.find_first_of(" \t,\"", pos);
        pos = line.find_first_not_of(" \t,", pos);
        if (pos == std::string::npos)
            return;

        Tools::addLog("---\r\n", true);
        Tools::addLog("Step 0: " + line, true);

        line_tr = line.substr(pos);

        Tools::addLog("Step 1: " + line_tr, true);
    }

    {
        std::regex r1("([\\w\\.]+|\".*?\")", std::regex::optimize);
        std::sregex_iterator next(line_tr.begin(), line_tr.end(), r1);
        std::sregex_iterator end;
        std::smatch found;

        int ctr = -1;
        while (next != end && ctr != pos_in_expression)
        {
            found = *next;
            next++;
            ctr++;
        }

        line_tr = found[1].str();
        pos += found.position(1);

        Tools::addLog("Step 2: " + line_tr, true);
    }

    {
        std::regex r2("\"(.*?)\"", std::regex::optimize);
        std::smatch found;
        std::regex_search(line_tr, found, r2);
        if (!found.empty())
        {
            line_tr = found[1].str();
            pos += 1;
        }

        Tools::addLog("Step 3: " + line_tr, true);
    }

    {
        auto search = merger->getDict(text_type).find(line_tr);
        if (search != merger->getDict(text_type).end())
        {
            new_text = search->second;
        }
        else
        {
            for (const auto & elem : merger->getDict(text_type))
            {
                if (Tools::caseInsensitiveStringCmp(line_tr, elem.first))
                {
                    new_text = elem.second;
                }
            }
        }

        Tools::addLog("Step 4: " + new_text, true);
    }

    {
        new_line.erase(pos, line_tr.size());
        new_line.insert(pos, new_text);

        Tools::addLog("Step 5: " + new_line, true);
    }

    {
        if (new_compiled.empty())
            return;

        pos_c = new_compiled.find(line_tr, pos_c);

        // BUG: make sure found is valid

        if (pos_c != std::string::npos)
        {
            pos_c -= 1;
            new_compiled.erase(pos_c, 1);
            new_compiled.insert(pos_c, Tools::convertUIntToStringByteArray(new_text.size()).substr(0, 1));
            pos_c += 1;
            new_compiled.erase(pos_c, new_text.size());
            new_compiled.insert(pos_c, new_text);

            if (0/*is_getpccell*/)
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

    }

    is_done = true;
}
