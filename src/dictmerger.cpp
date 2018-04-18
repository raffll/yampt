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
        DictReader reader;
        reader.readFile(elem);
        dict_coll.push_back(reader);
        log += reader.getLog();
    }
    status = true; // Not used
}

//----------------------------------------------------------
void DictMerger::mergeDict()
{
    if(status == true)
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
                        makeLog(yampt::type_name[type],
                                elem.first, elem.second,
                                search->second,
                                "Replaced by next dictionaries",
                                dict_coll[i].getNameFull());
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
        printLog();
    }
}

//----------------------------------------------------------
void DictMerger::makeLog(const std::string &id,
                         const std::string &unique_text,
                         const std::string &friendly_old,
                         const std::string &friendly_new,
                         const std::string &comment,
                         const std::string &name)
{
    log += "<log>\r\n";
    log += "\t<file>" + name + "</file>\r\n";
    log += "\t<status>" + comment + "</status>\r\n";
    log += "\t<id>" + id + "</id>\r\n";
    log += "\t<key>" + unique_text + "</key>\r\n";
    log += "\t<old>" + friendly_old + "</old>\r\n";
    log += "\t<new>" + friendly_new + "</new>\r\n";
    log += "<log>\r\n";
}

//----------------------------------------------------------
void DictMerger::printLog()
{
    std::cout << "---------------------------------" << std::endl
              << "    Merged / Replaced / Identical" << std::endl
              << "---------------------------------" << std::endl
              << std::setw(10) << std::to_string(counter_merged) << " / "
              << std::setw(8) << std::to_string(counter_replaced) << " / "
              << std::setw(9) << std::to_string(counter_identical) << std::endl
              << "---------------------------------" << std::endl;
}
