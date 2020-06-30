#include "dictmerger.hpp"

//----------------------------------------------------------
DictMerger::DictMerger()
{
    dict = Tools::initializeDict();
}

//----------------------------------------------------------
DictMerger::DictMerger(const std::vector<std::string> & paths)
{
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
                    // Not found in previous dictionary - inserted
                    dict.at(type).insert({ elem.first, elem.second });
                    counter_merged++;
                }
                else if (search != dict.at(type).end() &&
                         search->second != elem.second)
                {
                    // Found in previous dictionary - skipped
                    Tools::addLog("Warning: replaced " + Tools::getTypeName(type) + " record " + elem.first + "\r\n");
                    counter_replaced++;
                }
                else
                {
                    // Found in previous dictionary - identical, skipped
                    counter_identical++;
                }
            }
        }
    }

    if (readers.size() == 1)
    {
        Tools::addLog("--> Sorting complete!\r\n");
    }
    else
    {
        Tools::addLog("--> Merging complete!\r\n");
    }
}

//----------------------------------------------------------
void DictMerger::findDuplicateValues(Tools::RecType type)
{
    std::set<std::string> val_texts;
    std::string val_text_lc;
    for (const auto & elem : dict.at(type))
    {
        val_text_lc = elem.second;
        transform(val_text_lc.begin(), val_text_lc.end(),
                  val_text_lc.begin(), ::tolower);

        if (val_texts.insert(val_text_lc).second)
            continue;

        Tools::addLog("Warning: duplicate " + Tools::getTypeName(type) + " value " + elem.second + "\r\n");
    }
}

//----------------------------------------------------------
void DictMerger::findUnusedINFO()
{
    std::string test;
    bool found;
    size_t beg;
    size_t end;
    for (const auto & info : dict[Tools::RecType::INFO])
    {
        found = false;
        test = info.first;
        if (test.size() > 1 && test.substr(0, 1) == "T")
        {
            beg = test.find("^") + 1;
            end = test.find_last_of("^");
            test = test.substr(beg, end - beg);
            for (const auto & dial : dict[Tools::RecType::DIAL])
            {
                if (test == dial.second)
                {
                    found = true;
                }
            }

            if (!found)
            {
                Tools::addLog("Warning: dialog topic not found " + info.first + "\r\n");
            }
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
