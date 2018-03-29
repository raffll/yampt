#include "DictCreator.hpp"

//----------------------------------------------------------
DictCreator::DictCreator(std::string path_n)
{
	esm_ptr = &esm_n;
	message_ptr = &message_n;

    mode = yampt::ins_mode::RAW;

	esm_n.readFile(path_n);

	if(esm_n.getStatus() == true)
	{
		status = true;
		basic_mode = true;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(std::string path_n, std::string path_f)
{
	esm_ptr = &esm_f;
	message_ptr = &message_f;

    mode = yampt::ins_mode::BASE;

	esm_n.readFile(path_n);
	esm_f.readFile(path_f);

	if(esm_n.getStatus() == true && esm_f.getStatus() == true)
	{
		if(compareMasterFiles())
		{
			basic_mode = true;
		}
		else
		{
			basic_mode = false;
		}
		status = true;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(std::string path_n, DictMerger &merger, yampt::ins_mode mode)
{
	this->merger = &merger;
	this->mode = mode;

	esm_ptr = &esm_n;
	message_ptr = &message_n;

	esm_n.readFile(path_n);

	if(esm_n.getStatus() == true && merger.getStatus() == true)
	{
		status = true;
		basic_mode = true;
	}
}

//----------------------------------------------------------
void DictCreator::makeDict()
{
	if(basic_mode == true)
	{
		makeDictBasic();
	}
	else
	{
		makeDictExtended();
	}
}

//----------------------------------------------------------
void DictCreator::makeDictBasic()
{
	if(status == true)
	{
		printLogHeader();

		makeDictCELL();
		makeDictDefaultCELL();
		makeDictRegionCELL();
		makeDictGMST();
		makeDictFNAM();
		makeDictDESC();
		makeDictTEXT();
		makeDictRNAM();
		makeDictINDX();
		makeDictDIAL();
		makeDictINFO();
		makeDictBNAM();
		makeDictSCPT();

        std::cout << "----------------------------------------------" << std::endl;
	}
}

//----------------------------------------------------------
void DictCreator::makeDictExtended()
{
	if(status == true)
	{
		printLogHeader();

		makeDictCELLExtended();
		makeDictDefaultCELLExtended();
		makeDictRegionCELLExtended();
		makeDictGMST();
		makeDictFNAM();
		makeDictDESC();
		makeDictTEXT();
		makeDictRNAM();
		makeDictINDX();
		makeDictDIALExtended();
		makeDictINFO();
		makeDictBNAMExtended();
		makeDictSCPTExtended();

        std::cout << "----------------------------------------------" << std::endl;
	}
}

//----------------------------------------------------------
void DictCreator::printLogHeader()
{
    std::cout << "----------------------------------------------" << std::endl
         << "         Created / Doubled / Identical /   All" << std::endl
         << "----------------------------------------------" << std::endl;
}

//----------------------------------------------------------
void DictCreator::printLog(std::string id)
{
    std::string id_text = id;
	id_text.resize(9, ' ');
    std::cout << id_text
         << std::setw(7) << std::to_string(counter_created) << " / "
         << std::setw(7) << std::to_string(counter_doubled) << " / "
         << std::setw(8) << std::to_string(counter_identical) << " / "
         << std::setw(6) << std::to_string(counter_all) << std::endl;
}

//----------------------------------------------------------
bool DictCreator::compareMasterFiles()
{
    std::string esm_n_compare;
    std::string esm_f_compare;

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_n_compare += esm_n.getRecId();
	}

	for(size_t i = 0; i < esm_f.getRecColl().size(); ++i)
	{
		esm_f.setRec(i);
		esm_f_compare += esm_f.getRecId();
	}

	if(esm_n_compare != esm_f_compare)
	{
        std::cout << "--> Files have records in different order!" << std::endl;
        std::cout << "    If any doubled records will be created," << std::endl;
        std::cout <<	"    please check DIAL and CELL dictionary manually!" << std::endl;
        std::cout << "    Problematic records are marked as DOUBLED." << std::endl;
        std::cout << "    Please be patient..." << std::endl;
		return false;
	}
	else
	{
		return true;
	}
}

//----------------------------------------------------------
void DictCreator::resetCounters()
{
	counter_created = 0;
	counter_doubled = 0;
	counter_identical = 0;
	counter_all = 0;
}

//----------------------------------------------------------
void DictCreator::validateRecord(const std::string &unique_key, const std::string &friendly, yampt::rec_type type)
{
	counter_all++;

	// Insert without special cases
    if(mode == yampt::ins_mode::RAW || mode == yampt::ins_mode::BASE)
	{
		insertRecord(unique_key, friendly, type);
	}

	// For CELL, DIAL, BNAM, SCTX find corresponding record in dictionary given by user
    if(mode == yampt::ins_mode::ALL)
	{
        if(type == yampt::rec_type::CELL ||
           type == yampt::rec_type::DIAL ||
           type == yampt::rec_type::BNAM ||
           type == yampt::rec_type::SCTX)
		{
			auto search = merger->getDict()[type].find(unique_key);
			if(search != merger->getDict()[type].end())
			{
				insertRecord(unique_key, search->second, type);
			}
			else
			{
				insertRecord(unique_key, friendly, type);
			}
		}
		else
		{
			insertRecord(unique_key, friendly, type);
		}
	}

	// Insert only ones not found in dictionary given by user
    if(mode == yampt::ins_mode::NOTFOUND)
	{
		auto search = merger->getDict()[type].find(unique_key);
		if(search == merger->getDict()[type].end())
		{
			insertRecord(unique_key, friendly, type);
		}
	}

	// Insert only with changed friendly text compared to dictionary given by user
    if(mode == yampt::ins_mode::CHANGED)
	{
        if(type == yampt::rec_type::GMST ||
           type == yampt::rec_type::FNAM ||
           type == yampt::rec_type::DESC ||
           type == yampt::rec_type::TEXT ||
           type == yampt::rec_type::RNAM ||
           type == yampt::rec_type::INDX ||
           type == yampt::rec_type::INFO)
		{
			auto search = merger->getDict()[type].find(unique_key);
			if(search != merger->getDict()[type].end())
			{
				if(search->second != friendly)
				{
					insertRecord(unique_key, friendly, type);
				}
			}
		}
	}
}

//----------------------------------------------------------
void DictCreator::insertRecord(const std::string &unique_key, const std::string &friendly, yampt::rec_type type)
{
	if(dict[type].insert({unique_key, friendly}).second == true)
	{
		counter_created++;
	}
	else
	{
		auto search = dict[type].find(unique_key);
		if(friendly != search->second)
		{
            std::string temp = unique_key + "<DOUBLED_" + std::to_string(counter_doubled + 1) + ">";
			if(dict[type].insert({temp, friendly}).second == true)
			{
				counter_doubled++;
			}
			else
			{
				counter_identical++;
			}
		}
		else
		{
			counter_identical++;
		}
	}
}

//----------------------------------------------------------
std::string DictCreator::dialTranslator(std::string to_translate)
{
    if(mode == yampt::ins_mode::ALL ||
       mode == yampt::ins_mode::NOTFOUND ||
       mode == yampt::ins_mode::CHANGED)
	{
        auto search = merger->getDict()[yampt::rec_type::DIAL].find("DIAL" + yampt::sep[0] + to_translate);
        if(search != merger->getDict()[yampt::rec_type::DIAL].end())
		{
			return search->second;
		}
	}
	return to_translate;
}

//----------------------------------------------------------
std::vector<std::string> DictCreator::makeMessageColl(const std::string &script_text)
{
    std::vector<std::string> message;
	bool found;
    std::string line;
    std::string line_lc;
	size_t pos;

    std::istringstream ss(script_text);

    while(std::getline(ss, line))
	{
		found = false;
		eraseCarriageReturnChar(line);
		line_lc = line;
		transform(line_lc.begin(), line_lc.end(),
			  line_lc.begin(), ::tolower);

        for(auto const &elem : yampt::key_message)
		{
			if(found == false)
			{
				pos = line_lc.find(elem);
                if(pos != std::string::npos &&
                   line.rfind(";", pos) == std::string::npos)
				{
					message.push_back(line);
					found = true;
					break;
				}
			}
		}
	}
	return message;
}

//----------------------------------------------------------
void DictCreator::makeBinaryDump()
{
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_n.setDump();
		dump += esm_n.getRecId() + "\r\n";
		dump += esm_n.getDump() + "\r\n";
	}
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "CELL")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("NAME");

			esm_f.setRec(i);
			esm_f.setFriendly("NAME");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
                validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
					       esm_n.getFriendly(),
                           yampt::rec_type::CELL);
			}
		}
	}

	printLog("CELL");
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtended()
{
	resetCounters();
    std::string pattern;
    std::string match;
	bool found = false;

    std::cout << "CELL in progress..." << std::flush;

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "CELL")
		{
			esm_n.setUnique("NAME");

			if(esm_n.getUniqueStatus() == true)
			{
				found = false;

				// Pattern is the DATA and combined id of all objects in cell
				pattern = "";

				esm_n.setFriendly("DATA", false, false);
				pattern += esm_n.getFriendly();

				esm_n.setFriendly("NAME", false, false);

				while(esm_n.getFriendlyStatus() == true)
				{
					esm_n.setFriendly("NAME", true, true);
					pattern += esm_n.getFriendly();
				}

				// Search for match in every CELL record
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);

					if(esm_f.getRecId() == "CELL")
					{
						match = "";

						esm_f.setFriendly("DATA", false, false);
						match += esm_f.getFriendly();

						esm_f.setFriendly("NAME", false, false);

						while(esm_f.getFriendlyStatus() == true)
						{
							esm_f.setFriendly("NAME", true, true);
							match += esm_f.getFriendly();
						}

						if(match == pattern)
						{
							found = true;
							break;
						}
					}
				}

				esm_n.setFriendly("NAME");

				if(found == true)
				{
					esm_f.setFriendly("NAME");
				}
				else
				{
					esm_f.setFriendly("<NOTFOUND>");
				}

				if(esm_n.getFriendlyStatus() == true)
				{
                    validateRecord("CELL" + yampt::sep[0] + esm_f.getFriendly(),
						       esm_n.getFriendly(),
                               yampt::rec_type::CELL);
				}

				if(counter_created % 25 == 0)
				{
                    std::cout << "." << std::flush;
				}
			}
		}
	}

    std::cout << std::endl;

	printLog("CELL");
}

//----------------------------------------------------------
void DictCreator::makeDictDefaultCELL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");

			esm_f.setRec(i);
			esm_f.setFriendly("STRV");

                        if(esm_n.getUnique() == "sDefaultCellname" &&
			   esm_n.getFriendlyStatus() == true)
                        {
                validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
						esm_n.getFriendly(),
                        yampt::rec_type::CELL);
                        }
		}
	}

	printLog("+ Default");
}

//----------------------------------------------------------
void DictCreator::makeDictDefaultCELLExtended()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");

                        if(esm_n.getUnique() == "sDefaultCellname" &&
                           esm_n.getFriendlyStatus() == true)
                        {
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);
					if(esm_f.getRecId() == "GMST")
					{
						esm_f.setUnique("NAME");
						esm_f.setFriendly("STRV");

						if(esm_f.getUnique() == "sDefaultCellname" &&
						   esm_f.getFriendlyStatus() == true)
						{
                            validateRecord("CELL" + yampt::sep[0] + esm_f.getFriendly(),
									esm_n.getFriendly(),
                                    yampt::rec_type::CELL);
							break;
						}
					}
				}
			}
		}
	}

	printLog("+ Default");
}

//----------------------------------------------------------
void DictCreator::makeDictRegionCELL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "REGN")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("FNAM");

			esm_f.setRec(i);
			esm_f.setFriendly("FNAM");

                        if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
                        {
                validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
						esm_n.getFriendly(),
                        yampt::rec_type::CELL);
                        }
		}
	}

	printLog("+ Region ");
}

//----------------------------------------------------------
void DictCreator::makeDictRegionCELLExtended()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "REGN")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("FNAM");

                        if(esm_n.getUniqueStatus() == true &&
                           esm_n.getFriendlyStatus() == true)
                        {
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);
					if(esm_f.getRecId() == "REGN")
					{
						esm_f.setUnique("NAME");
						esm_f.setFriendly("FNAM");

						if(esm_f.getUnique() == esm_n.getUnique() &&
						   esm_f.getFriendlyStatus() == true)
						{
                            validateRecord("CELL" + yampt::sep[0] + esm_f.getFriendly(),
									esm_n.getFriendly(),
                                    yampt::rec_type::CELL);
							break;
						}
					}
				}
			}
		}
	}

	printLog("+ Region ");
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true &&
			   esm_n.getUnique().substr(0, 1) == "s")	// Make sure is string
			{
                validateRecord("GMST" + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
                           yampt::rec_type::GMST);
			}
		}
	}

	printLog("GMST");
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "ACTI" ||
		   esm_n.getRecId() == "ALCH" ||
		   esm_n.getRecId() == "APPA" ||
		   esm_n.getRecId() == "ARMO" ||
		   esm_n.getRecId() == "BOOK" ||
		   esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "CLOT" ||
		   esm_n.getRecId() == "CONT" ||
		   esm_n.getRecId() == "CREA" ||
		   esm_n.getRecId() == "DOOR" ||
		   esm_n.getRecId() == "FACT" ||
		   esm_n.getRecId() == "INGR" ||
		   esm_n.getRecId() == "LIGH" ||
		   esm_n.getRecId() == "LOCK" ||
		   esm_n.getRecId() == "MISC" ||
		   esm_n.getRecId() == "NPC_" ||
		   esm_n.getRecId() == "PROB" ||
		   esm_n.getRecId() == "RACE" ||
		   esm_n.getRecId() == "REGN" ||
		   esm_n.getRecId() == "REPA" ||
		   esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "SPEL" ||
		   esm_n.getRecId() == "WEAP")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("FNAM");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true &&
			   esm_n.getUnique() != "player")	// Skip player name
			{
                validateRecord("FNAM" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
                           yampt::rec_type::FNAM);
			}
		}
	}

	printLog("FNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "RACE")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("DESC");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
                validateRecord("DESC" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
                           yampt::rec_type::DESC);
			}
		}
	}

	printLog("DESC");
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "BOOK")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("TEXT");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
                validateRecord("TEXT" + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
                           yampt::rec_type::TEXT);
			}
		}
	}

	printLog("TEXT");
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "FACT")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("RNAM");

			if(esm_n.getUniqueStatus() == true)
			{
				while(esm_n.getFriendlyStatus() == true)
				{
                    validateRecord("RNAM" + yampt::sep[0] + esm_n.getUnique() + yampt::sep[0] + std::to_string(esm_n.getFriendlyCounter()),
						       esm_n.getFriendly(),
                               yampt::rec_type::RNAM);

					esm_n.setFriendly("RNAM", true, true);
				}
			}
		}
	}

	printLog("RNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "MGEF")
		{
			esm_n.setUnique("INDX");
			esm_n.setFriendly("DESC");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
                validateRecord("INDX" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
                           yampt::rec_type::INDX);
			}
		}
	}

	printLog("INDX");
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);

		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setUnique("DATA");
			esm_n.setFriendly("NAME");
			esm_f.setFriendly("NAME");

			if(esm_n.getUnique() == "T" &&
			   esm_n.getFriendlyStatus() == true)
			{
                validateRecord("DIAL" + yampt::sep[0] + esm_ptr->getFriendly(),
					       esm_n.getFriendly(),
                           yampt::rec_type::DIAL);
			}
		}
	}

	printLog("DIAL");
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtended()
{
	resetCounters();
    std::string pattern;
    std::string match;

    std::cout << "DIAL in progress...";

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setUnique("DATA");

			if(esm_n.getUnique() == "T")
			{
				esm_n.setRec(i + 1);

				esm_n.setFriendly("INAM", false, false);
				pattern = esm_n.getFriendly();

				esm_n.setFriendly("SCVR", false, false);
				pattern += esm_n.getFriendly();

				esm_n.setRec(i);
				esm_n.setFriendly("NAME");

                //std::cout << "Pattern: " << pattern << std::endl;

				// Search for all match
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);

					if(esm_f.getRecId() == "DIAL")
					{
						esm_f.setRec(k + 1);

						esm_f.setFriendly("INAM", false, false);
						match = esm_f.getFriendly();

						esm_f.setFriendly("SCVR", false, false);
						match += esm_f.getFriendly();

						if(match == pattern)
						{
							esm_f.setRec(k);
							esm_f.setFriendly("NAME");

                            //std::cout << "Match: " << match << std::endl;

							break;
						}
					}
				}

				if(esm_n.getFriendlyStatus() == true &&
				   esm_f.getFriendlyStatus() == true)
				{
                    //std::cout << esm_f.getFriendly() << " <<< " << esm_n.getFriendly() << std::endl;

                    validateRecord("DIAL" + yampt::sep[0] + esm_f.getFriendly(),
						       esm_n.getFriendly(),
                               yampt::rec_type::CELL);
				}

				if(counter_created % 25 == 0)
				{
                    std::cout << "." << std::flush;
				}
			}
		}
	}

    std::cout << std::endl;

	printLog("DIAL");
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
	resetCounters();
    std::string dialog_topic;

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setUnique("DATA");
			esm_n.setFriendly("NAME");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
                dialog_topic = esm_n.getUnique() + yampt::sep[0] + dialTranslator(esm_n.getFriendly());
			}
			else
			{
				dialog_topic = "<NOTFOUND>";
			}
		}

		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setUnique("INAM");
			esm_n.setFriendly("NAME");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
                validateRecord("INFO" + yampt::sep[0] + dialog_topic + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
                           yampt::rec_type::INFO);
			}
		}
	}

	printLog("INFO");
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);

		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setFriendly("BNAM");
			esm_f.setFriendly("BNAM");

			if(esm_n.getFriendlyStatus() == true)
			{
				message_n = makeMessageColl(esm_n.getFriendly());
				message_f = makeMessageColl(esm_f.getFriendly());

				for(size_t k = 0; k < message_n.size(); ++k)
				{
                    validateRecord("BNAM" + yampt::sep[0] + message_ptr->at(k),
						       message_n.at(k),
                               yampt::rec_type::BNAM);
				}
			}
		}
	}

	printLog("BNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtended()
{
	resetCounters();

    std::cout << "BNAM in progress...";

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setUnique("INAM");
			esm_n.setFriendly("BNAM");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
				for(size_t j = 0; j < esm_f.getRecColl().size(); ++j)
				{
					esm_f.setRec(j);

					if(esm_f.getRecId() == "INFO")
					{
						esm_f.setUnique("INAM");
						esm_f.setFriendly("BNAM");

						if(esm_f.getUnique() == esm_n.getUnique() &&
						   esm_f.getFriendlyStatus() == true)
						{
							message_n = makeMessageColl(esm_n.getFriendly());
							message_f = makeMessageColl(esm_f.getFriendly());

							if(message_n.size() == message_f.size())
							{
								for(size_t k = 0; k < message_n.size(); ++k)
								{
                                    //std::cout << "---" << std::endl;
                                    //std::cout << message_f.at(k) << std::endl;
                                    //std::cout << message_n.at(k) << std::endl;

                                    validateRecord("BNAM" + yampt::sep[0] + message_f.at(k),
										       message_n.at(k),
                                               yampt::rec_type::BNAM);

									if(counter_created % 25 == 0)
									{
                                        std::cout << "." << std::flush;
									}
								}
								break;
							}
						}
					}
				}
			}
		}
	}

    std::cout << std::endl;

	printLog("BNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);

		if(esm_n.getRecId() == "SCPT")
		{
			esm_n.setFriendly("SCTX");
			esm_f.setFriendly("SCTX");

			if(esm_n.getFriendlyStatus() == true)
			{
				message_n = makeMessageColl(esm_n.getFriendly());
				message_f = makeMessageColl(esm_f.getFriendly());

				for(size_t k = 0; k < message_n.size(); ++k)
				{
                    validateRecord("SCTX" + yampt::sep[0] + message_ptr->at(k),
						       message_n.at(k),
                               yampt::rec_type::SCTX);
				}
			}
		}
	}

	printLog("SCTX");
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtended()
{
	resetCounters();

    std::cout << "SCTX in progress...";

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "SCPT")
		{
			esm_n.setUnique("SCHD");
			esm_n.setFriendly("SCTX");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true)
			{
				for(size_t j = 0; j < esm_f.getRecColl().size(); ++j)
				{
					esm_f.setRec(j);

					if(esm_f.getRecId() == "SCPT")
					{
						esm_f.setUnique("SCHD");
						esm_f.setFriendly("SCTX");

						if(esm_f.getUnique() == esm_n.getUnique() &&
						   esm_f.getFriendlyStatus() == true)
						{
							message_n = makeMessageColl(esm_n.getFriendly());
							message_f = makeMessageColl(esm_f.getFriendly());

							if(message_n.size() == message_f.size())
							{
								for(size_t k = 0; k < message_n.size(); ++k)
								{
                                    //std::cout << "---" << std::endl;
                                    //std::cout << message_f.at(k) << std::endl;
                                    //std::cout << message_n.at(k) << std::endl;

                                    validateRecord("SCTX" + yampt::sep[0] + message_f.at(k),
										       message_n.at(k),
                                               yampt::rec_type::BNAM);

									if(counter_created % 25 == 0)
									{
                                        std::cout << "." << std::flush;
									}
								}
								break;
							}
						}
					}
				}
			}
		}
	}

    std::cout << std::endl;

	printLog("SCTX");
}
