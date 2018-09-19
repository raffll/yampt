#include "dictmerger.hpp"

//----------------------------------------------------------
DictMerger::DictMerger()
{

}

//----------------------------------------------------------
DictMerger::DictMerger(const std::vector<std::string> &path)
{
    for(const auto &elem : path)
    {
        DictReader reader(elem);
        dict_coll.push_back(reader);
    }
    mergeDict();
    printSummaryLog();
}

//----------------------------------------------------------
void DictMerger::addRecord(const yampt::rec_type type,
                           const std::string &unique_text,
                           const std::string &friendly_text)
{
    dict[type].insert({unique_text, friendly_text});
}

//----------------------------------------------------------
void DictMerger::mergeDict()
{
    for(size_t i = 0; i < dict_coll.size(); ++i)
    {
        for(size_t type = 0; type < dict.size(); ++type) // dict_t size
        {
            for(auto &elem : dict_coll[i].getDict(type))
            {
                auto search = dict[type].find(elem.first);
                if(search == dict[type].end())
                {
                    // Not found in previous dictionary - inserted
                    dict[type].insert({elem.first, elem.second});
                    counter_merged++;
                }
                else if(search != dict[type].end() &&
                        search->second != elem.second)
                {
                    // Found in previous dictionary - skipped
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

    if(dict_coll.size() == 1)
    {
        std::cout << "--> Sorting complete!\r\n";
    }
    else
    {
        std::cout << "--> Merging complete!\r\n";
    }
}

//----------------------------------------------------------
void DictMerger::printSummaryLog()
{
    std::cout << "---------------------------------" << std::endl
              << "    Merged / Replaced / Identical" << std::endl
              << "---------------------------------" << std::endl
              << std::setw(10) << std::to_string(counter_merged) << " / "
              << std::setw(8) << std::to_string(counter_replaced) << " / "
              << std::setw(9) << std::to_string(counter_identical) << std::endl
              << "---------------------------------" << std::endl;
}
