#include "dictmerger.hpp"


DictMerger::DictMerger()
{
    dict = tools_t::initializeDict();
}


DictMerger::DictMerger(const std::vector<std::string> & paths)
{
    dict = tools_t::initializeDict();

    for (const auto & path : paths)
    {
        DictReader reader(path);
        readers.push_back(reader);
    }

    mergeDict();
    findDuplicateValues(tools_t::rec_type_t::CELL);
    findDuplicateValues(tools_t::rec_type_t::DIAL);
    findUnusedINFO();
    printSummaryLog();
}


void DictMerger::addRecord(
    const tools_t::rec_type_t type,
    const std::string & key_text,
    const std::string & val_text)
{
    tools_t::RecordEntry entry;
    entry.key_text = key_text;
    entry.new_text = val_text;
    dict.at(type).insert(entry);
}


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
                    if (type != tools_t::rec_type_t::Glossary)
                        tools_t::addLog("Warning: replaced " + tools_t::type2Str(type) + " record " + entry.key_text + "\r\n");
                    counter_replaced++;
                }
                else
                {
                    counter_identical++;
                }
            }
        }
    }

    tools_t::addLog("--> Merging complete!\r\n");
}


void DictMerger::findDuplicateValues(tools_t::rec_type_t type)
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

        tools_t::addLog("Warning: duplicate " + tools_t::type2Str(type) + " value " + entry.new_text + "\r\n");
    }
}


void DictMerger::findUnusedINFO()
{
    for (const auto & info : dict.at(tools_t::rec_type_t::INFO).records)
    {
        bool found = false;
        std::string text = info.key_text;
        if (text.size() < 1 || text.substr(0, 1) != "T")
            continue;

        size_t beg = text.find("^") + 1;
        size_t end = text.find_last_of("^");
        text = text.substr(beg, end - beg);

        for (const auto & dial : dict.at(tools_t::rec_type_t::DIAL).records)
        {
            if (text != dial.new_text)
                continue;

            found = true;
        }

        if (!found)
        {
            tools_t::addLog("Warning: dialog topic not found " + info.key_text + "\r\n");
        }
    }
}


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

    tools_t::addLog(ss.str());
}
