#include "dictmerger.hpp"

//----------------------------------------------------------
DictMerger::DictMerger()
{

}

//----------------------------------------------------------
DictMerger::DictMerger(const std::vector<std::string> &path,
                       bool ext_log)
    : ext_log(ext_log)
{
    for(const auto &elem : path)
    {
        DictReader reader(elem);
        dict_coll.push_back(reader);
    }
    mergeDict();
    if(ext_log == true)
    {
        findDuplicateFriendlyText(yampt::rec_type::CELL);
        findDuplicateFriendlyText(yampt::rec_type::DIAL);
        findUnusedINFO();
    }
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
                    if(ext_log == true)
                    {
                        tools.addLog("Replaced record in " + yampt::type_name[type] + ": " + elem.first);
                    }
                    counter_replaced++;
                }
                else
                {
                    // Found in previous dictionary - identical, skipped
                    if(ext_log == true)
                    {
                        tools.addLog("Identical record in " + yampt::type_name[type] + ": " + elem.first);
                    }
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
void DictMerger::findDuplicateFriendlyText(yampt::rec_type type)
{
    std::set<std::string> test_set;
    std::string test;
    for(const auto &elem : dict[type])
    {
        test = elem.second;
        transform(test.begin(), test.end(),
                  test.begin(), ::tolower);
        if(test_set.insert(test).second == false)
        {
            tools.addLog("Duplicate value in " + yampt::type_name[type] + ": " + elem.second);
        }
    }
}

//----------------------------------------------------------
void DictMerger::findUnusedINFO()
{
    std::string test;
    bool found;
    size_t beg;
    size_t end;
    for(const auto &info : dict[yampt::rec_type::INFO])
    {
        found = false;
        test = info.first;
        if(test.size() > 1 && test.substr(0, 1) == "T")
        {
            beg = test.find("^") + 1;
            end = test.find_last_of("^");
            test = test.substr(beg, end - beg);
            for(const auto &dial : dict[yampt::rec_type::DIAL])
            {
                if(test == dial.second)
                {
                    found = true;
                }
            }
            if(found == false)
            {
                tools.addLog("Unused INFO record: " + info.first);
            }
        }
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
    if(!tools.getLog().empty())
    {
        std::cout << tools.getLog()
                  << "---------------------------------" << std::endl;
        tools.clearLog();
    }
}
