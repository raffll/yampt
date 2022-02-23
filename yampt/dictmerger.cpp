#include "dictmerger.hpp"

//----------------------------------------------------------
DictMerger::DictMerger()
{
    dict = Tools::initializeDict();
}

//----------------------------------------------------------
DictMerger::DictMerger(const std::vector<std::string> & paths)
{
    Tools::addLog("-> Start merging dictionaries...\r\n");
    dict = Tools::initializeDict();

    for (const auto & path : paths)
    {
        DictReader reader(path);
        readers.push_back(reader);
    }

    mergeDict();
    findDuplicateValues(Tools::RecType::CELL);
    findDuplicateValues(Tools::RecType::DIAL);
    findUnusedINFO();
    printSummaryLog();
    Tools::addLog("-> Done!\r\n");
}

//----------------------------------------------------------
void DictMerger::addRecord(
    const Tools::RecType type,
    const std::string & key_text,
    const std::string & val_text)
{
    dict.at(type).insert({ key_text, val_text });
}

//----------------------------------------------------------
void DictMerger::mergeDict()
{
    for (const auto & reader : readers)
    {
        for (const auto & chapter : reader.getDict())
        {
            for (const auto & elem : chapter.second)
            {
                const auto & type = chapter.first;
                auto search = dict.at(type).find(elem.first);
                if (search == dict.at(type).end())
                {
                    /* not found in previous dictionary - inserted */
                    dict.at(type).insert({ elem.first, elem.second });
                    counter_merged++;
                }
                else if (search != dict.at(type).end() &&
                         search->second != elem.second)
                {
                    /* found in previous dictionary - skipped */
                    if (type != Tools::RecType::Glossary)
                        Tools::addLog("Warning: replaced " + Tools::type2Str(type) + " record " + elem.first + "\r\n");
                    counter_replaced++;
                }
                else
                {
                    /* found in previous dictionary - identical, skipped */
                    counter_identical++;
                }
            }
        }
    }

    Tools::addLog("--> Merging complete!\r\n");
}

//----------------------------------------------------------
void DictMerger::findDuplicateValues(Tools::RecType type)
{
    std::set<std::string> texts;
    for (const auto & elem : dict.at(type))
    {
        std::string text_lc = elem.second;
        transform(
            text_lc.begin(), text_lc.end(),
            text_lc.begin(), ::tolower);

        if (texts.insert(text_lc).second)
            continue;

        Tools::addLog("Warning: duplicate " + Tools::type2Str(type) + " value " + elem.second + "\r\n");
    }
}

//----------------------------------------------------------
void DictMerger::findUnusedINFO()
{
    for (const auto & info : dict.at(Tools::RecType::INFO))
    {
        bool found = false;
        std::string text = info.first;
        if (text.size() < 1 || text.substr(0, 1) != "T")
            continue;

        size_t beg = text.find("^") + 1;
        size_t end = text.find_last_of("^");
        text = text.substr(beg, end - beg);

        for (const auto & dial : dict.at(Tools::RecType::DIAL))
        {
            if (text != dial.second)
                continue;

            found = true;
        }

        if (!found)
        {
            Tools::addLog("Warning: dialog topic not found " + info.first + "\r\n");
        }
    }
}

//----------------------------------------------------------
void DictMerger::printSummaryLog()
{
    std::ostringstream ss;
    ss
        << "---------------------------------" << std::endl
        << "    Merged / Replaced / Identical" << std::endl
        << "---------------------------------" << std::endl
        << std::setw(10) << std::to_string(counter_merged) << " / "
        << std::setw(8) << std::to_string(counter_replaced) << " / "
        << std::setw(9) << std::to_string(counter_identical) << std::endl
        << "---------------------------------" << std::endl;

    Tools::addLog(ss.str());
}
