#include "DictMerger.hpp"

//----------------------------------------------------------
DictMerger::DictMerger()
{

}

//----------------------------------------------------------
DictMerger::DictMerger(std::vector<std::string> &path)
{
	for(auto &elem : path)
	{
		DictReader reader;
		reader.readFile(elem);
		dict_coll.push_back(reader);
		log += reader.getLog();
	}
	status = true;
}

//----------------------------------------------------------
void DictMerger::mergeDict()
{
	if(status == true)
	{
		for(size_t i = 0; i < dict_coll.size(); ++i)
		{
			makeLogHeader(i);

			for(size_t k = 0; k < dict_coll[i].getDict().size(); ++k)
			{
				for(auto &elem : dict_coll[i].getDict()[k])
				{
					auto search = dict[k].find(elem.first);
					if(search == dict[k].end())
					{
						// Not found in previous dictionary - inserted
						dict[k].insert({elem.first, elem.second});
						counter_merged++;
					}
					else if(search != dict[k].end() &&
						search->second != elem.second)
					{
						// Found in previous dictionary - skipped
                        merger_log_ptr = &yampt::merger_log[0];
						makeLog(elem.first, elem.second, search->second);
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
void DictMerger::findDiff()
{
	if(status == true && dict_coll.size() == 2)
	{
		for(size_t type = 0; type < 11; type++)
		{
			for(auto &elem : dict_coll[0].getDict()[type])
			{
				auto search = dict_coll[1].getDict()[type].find(elem.first);
				if(search != dict_coll[1].getDict()[type].end())
				{
					if(search->second != elem.second)
					{
						diff[0][type].insert({elem.first, elem.second});
						diff[1][type].insert({search->first, search->second});
					}
				}
			}
		}
	}
}

//----------------------------------------------------------
void DictMerger::wordList()
{
	if(status == true)
	{
		log += "<!-- Creating word list... -->\r\n";
        log += yampt::sep_line + "\r\n";

        std::string word;
        std::string unnecessary = "'\";:?.,!()<>";

		for(size_t type = 0; type < 11; type++)
		{
			for(auto &elem : dict_coll[0].getDict()[type])
			{
                std::istringstream ss(elem.second);
                while(std::getline(ss, word, ' '))
				{
                    if(word.find_first_not_of(unnecessary) != std::string::npos)
					{
						word.substr(word.find_first_not_of(unnecessary));
					}

                    if(word.find_first_of(unnecessary) != std::string::npos)
					{
						word.erase(word.find_first_of(unnecessary));
					}
					dict[0].insert({word, ""});
				}
			}
		}

		for(auto &elem : dict[0])
		{
			log += elem.first + "\r\n";
		}
	}
}

//----------------------------------------------------------
void DictMerger::swapRecords()
{
	if(status == true)
	{
        std::string prefix;
        std::string suffix;

		for(size_t type = 0; type < 11; type++)
		{
            if(type == yampt::rec_type::CELL ||
               type == yampt::rec_type::DIAL ||
               type == yampt::rec_type::BNAM ||
               type == yampt::rec_type::SCTX)
			{
				for(auto &elem : dict_coll[0].getDict()[type])
				{
					prefix = elem.first.substr(0, 5);
					suffix = elem.first.substr(5);

					dict[type].insert({prefix + elem.second, suffix});
				}
			}
			else
			{
				for(auto &elem : dict_coll[0].getDict()[type])
				{
					dict[type].insert({elem.first, elem.second});
				}
			}
		}
	}
}

//----------------------------------------------------------
void DictMerger::makeLogHeader(size_t i)
{
	if(dict_coll.size() == 1)
	{
		log += "<!-- Nothing to merge... -->\r\n";
        log += yampt::sep_line + "\r\n";
	}
	else if(dict_coll.size() > 1 && i == 1)
	{
		log += "<!-- Merging " + dict_coll[i].getName() + " with " + dict_coll[i - 1].getName() + "... -->\r\n";
        log += yampt::sep_line + "\r\n";
	}
	else if(dict_coll.size() > 2 && i > 1)
	{
		log += "<!-- Merging " + dict_coll[i].getName() + " with previous dictionaries... -->\r\n";
        log += yampt::sep_line + "\r\n";
	}
}

//----------------------------------------------------------
void DictMerger::makeLog(const std::string unique_key, const std::string friendly_old, const std::string friendly_new)
{

	log += "<!-- " + *merger_log_ptr + " -->\r\n";
    log += yampt::sep[1] + unique_key + yampt::sep[2] + friendly_old + yampt::sep[3] + "\r\n";
    log += yampt::sep[1] + unique_key + yampt::sep[2] + friendly_new + yampt::sep[3] + "\r\n";
    log += yampt::sep_line + "\r\n";
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
