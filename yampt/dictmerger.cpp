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
    Tools::RecordEntry entry;
    entry.key_text = key_text;
    entry.new_text = val_text;
    dict.at(type).insert(entry);
}

//----------------------------------------------------------
void DictMerger::mergeDict()
{
    for (const auto & reader : readers)
    {
        for (const auto & chapter : reader.getDict())
        {
            const auto & type = chapter.first;
            for (const auto & entry : chapter.second.records)
            {
                auto * existing = dict.at(type).find(entry.key_text);
                if (existing == nullptr)
                {
                    dict.at(type).insert(entry);
                    counter_merged++;
                }
                else if (existing->new_text != entry.new_text)
                {
                    if (type != Tools::RecType::Glossary)
                        Tools::addLog("Warning: replaced " + Tools::type2Str(type) + " record " + entry.key_text + "\r\n");
                    counter_replaced++;
                }
                else
                {
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
    for (const auto & entry : dict.at(type).records)
    {
        std::string text_lc = entry.new_text;
        transform(
            text_lc.begin(), text_lc.end(),
            text_lc.begin(), ::tolower);

        if (texts.insert(text_lc).second)
            continue;

        Tools::addLog("Warning: duplicate " + Tools::type2Str(type) + " value " + entry.new_text + "\r\n");
    }
}

//----------------------------------------------------------
void DictMerger::findUnusedINFO()
{
    for (const auto & info : dict.at(Tools::RecType::INFO).records)
    {
        bool found = false;
        std::string text = info.key_text;
        if (text.size() < 1 || text.substr(0, 1) != "T")
            continue;

        size_t beg = text.find("^") + 1;
        size_t end = text.find_last_of("^");
        text = text.substr(beg, end - beg);

        for (const auto & dial : dict.at(Tools::RecType::DIAL).records)
        {
            if (text != dial.new_text)
                continue;

            found = true;
        }

        if (!found)
        {
            Tools::addLog("Warning: dialog topic not found " + info.key_text + "\r\n");
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
