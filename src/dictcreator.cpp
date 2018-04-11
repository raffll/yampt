#include "dictcreator.hpp"

//----------------------------------------------------------
DictCreator::DictCreator(const std::string &path_n)
    : esm_ptr(&esm_n),
      message_ptr(&message_n),
      mode(yampt::ins_mode::RAW)
{
    esm_n.readFile(path_n);
    if(esm_n.getStatus() == true)
    {
        status = true;
        basic_mode = true;
    }
}

//----------------------------------------------------------
DictCreator::DictCreator(const std::string &path_n,
                         const std::string &path_f)
    : esm_ptr(&esm_f),
      message_ptr(&message_f),
      mode(yampt::ins_mode::BASE)
{
    esm_n.readFile(path_n);
    esm_f.readFile(path_f);
    if(esm_n.getStatus() == true &&
       esm_f.getStatus() == true)
    {
        if(compareMasterFiles() == true)
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
DictCreator::DictCreator(const std::string &path_n,
                         DictMerger &merger,
                         const yampt::ins_mode mode)
    : esm_ptr(&esm_n),
      merger(&merger),
      message_ptr(&message_n),
      mode(mode)
{
    esm_n.readFile(path_n);
    if(esm_n.getStatus() == true &&
       this->merger->getStatus() == true)
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
        makeDictCELL();
        makeDictCELLWilderness();
        makeDictCELLRegion();
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
    }
}

//----------------------------------------------------------
void DictCreator::makeDictExtended()
{
    if(status == true)
    {
        makeDictCELLExtended();
        makeDictCELLExtendedWilderness();
        makeDictCELLExtendedRegion();
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
    }
}

//----------------------------------------------------------
void DictCreator::printLogLine(const yampt::rec_type type)
{
    std::string id = yampt::type_name[type];

    if(type == yampt::rec_type::Wilderness ||
       type == yampt::rec_type::Region)
    {
        id = "+ " + id;
    }
    id.resize(12, ' ');

    std::cout << id
              << std::setw(5) << std::to_string(counter_created) << " / ";

    if(type == yampt::rec_type::CELL ||
       type == yampt::rec_type::DIAL)
    {
        std::cout << std::setw(7) << std::to_string(counter_missing) << " / ";
    }
    else
    {
        std::cout << std::setw(7) << "-" << " / ";
    }
    std::cout << std::setw(8) << std::to_string(counter_identical) << " / "
              << std::setw(6) << std::to_string(counter_all) << std::endl;
}

//----------------------------------------------------------
bool DictCreator::compareMasterFiles()
{
    std::string sum_of_n;
    std::string sum_of_f;

    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        sum_of_n += esm_n.getRecordId();
    }

    for(size_t i = 0; i < esm_f.getRecordColl().size(); ++i)
    {
        esm_f.setRecordTo(i);
        sum_of_f += esm_f.getRecordId();
    }

    if(sum_of_n != sum_of_f)
    {
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
    counter_missing = 0;
    counter_doubled = 0;
    counter_identical = 0;
    counter_all = 0;
}

//----------------------------------------------------------
void DictCreator::validateRecord(const std::string &unique_text,
                                 const std::string &friendly_text,
                                 const yampt::rec_type type)
{
    counter_all++;

    // Insert without special cases
    if(mode == yampt::ins_mode::RAW ||
       mode == yampt::ins_mode::BASE)
    {
        insertRecord(unique_text, friendly_text, type);
    }

    // For CELL, DIAL, BNAM, SCTX find corresponding record in dictionary given by user
    if(mode == yampt::ins_mode::ALL)
    {
        if(type == yampt::rec_type::CELL ||
           type == yampt::rec_type::DIAL ||
           type == yampt::rec_type::BNAM ||
           type == yampt::rec_type::SCTX)
        {
            auto search = merger->getDict(type).find(unique_text);
            if(search != merger->getDict(type).end())
            {
                insertRecord(unique_text, search->second, type);
            }
            else
            {
                insertRecord(unique_text, friendly_text, type);
            }
        }
        else
        {
            insertRecord(unique_text, friendly_text, type);
        }
    }

    // Insert only ones not found in dictionary given by user
    if(mode == yampt::ins_mode::NOTFOUND)
    {
        auto search = merger->getDict(type).find(unique_text);
        if(search == merger->getDict(type).end())
        {
            insertRecord(unique_text, friendly_text, type);
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
            auto search = merger->getDict(type).find(unique_text);
            if(search != merger->getDict(type).end())
            {
                if(search->second != friendly_text)
                {
                    insertRecord(unique_text, friendly_text, type);
                }
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::insertRecord(const std::string &unique_text,
                               const std::string &friendly_text,
                               const yampt::rec_type type)
{
    if(dict[type].insert({unique_text, friendly_text}).second == true)
    {
        counter_created++;
    }
    else
    {
        auto search = dict[type].find(unique_text);
        if(friendly_text != search->second)
        {
            std::string unique_current = unique_text + yampt::err[0] + "DOUBLED_" + std::to_string(counter_doubled) + yampt::err[1];
            if(dict[type].insert({unique_current, friendly_text}).second == true)
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
        auto search = merger->getDict(yampt::rec_type::DIAL).find(to_translate);
        if(search != merger->getDict(yampt::rec_type::DIAL).end())
        {
            return search->second;
        }
    }
    return to_translate;
}

//----------------------------------------------------------
std::vector<std::string> DictCreator::makeMessageColl(const std::string &new_friendly)
{
    std::vector<std::string> message_coll;
    std::string line;
    std::string line_lc;
    std::istringstream ss(new_friendly);
    size_t keyword_pos;

    while(std::getline(ss, line))
    {
        std::set<size_t> keyword_pos_coll;
        line = tools.eraseCarriageReturnChar(line);
        line_lc = line;
        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

        for(size_t i = 0; i < yampt::keyword_list.size(); ++i)
        {
            keyword_pos = line_lc.find(yampt::keyword_list[i]);
            keyword_pos_coll.insert(keyword_pos);
        }

        if(*keyword_pos_coll.begin() != std::string::npos &&
           line.rfind(";", *keyword_pos_coll.begin()) == std::string::npos)
        {
            message_coll.push_back(line);
        }
    }
    return message_coll;
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "CELL")
        {
            esm_n.setUniqueTo("NAME"); // For Check, because unique text can't be empty
            esm_n.setFirstFriendlyTo("NAME");
            esm_ptr->setRecordTo(i);
            esm_ptr->setUniqueTo("NAME");
            esm_ptr->setFirstFriendlyTo("NAME");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               esm_ptr->getUniqueStatus() == true &&
               esm_ptr->getFriendlyStatus() == true)
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::CELL);
            }
        }
    }
    printLogLine(yampt::rec_type::CELL);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtended()
{
    std::vector<std::tuple<std::string, size_t, bool>> pattern_coll;
    std::map<std::string, size_t> match_coll;

    pattern_coll = makeDictCELLExtendedPattern();
    match_coll = makeDictCELLExtendedMatch();

    resetCounters();
    for(size_t i = 0; i < pattern_coll.size(); ++i)
    {
        auto search = match_coll.find(std::get<0>(pattern_coll[i]));
        if(search != match_coll.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setFirstFriendlyTo("NAME");
            esm_f.setRecordTo(std::get<1>(pattern_coll[i]));
            esm_f.setFirstFriendlyTo("NAME");
            if(esm_n.getFriendlyStatus() == true &&
               esm_f.getFriendlyStatus() == true)
            {
                validateRecord(esm_f.getFriendlyText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::CELL);
            }
            std::get<2>(pattern_coll[i]) = true;
        }
        else
        {
            counter_missing++;
        }
    }

    // Add missing records to dictionary
    for(size_t i = 0; i < pattern_coll.size(); ++i)
    {
        if(std::get<2>(pattern_coll[i]) == false)
        {
            esm_f.setRecordTo(std::get<1>(pattern_coll[i]));
            esm_f.setFirstFriendlyTo("NAME");
            if(esm_f.getFriendlyStatus() == true)
            {
                validateRecord(esm_f.getFriendlyText(),
                               yampt::err[0] + "MISSING" + yampt::err[1],
                        yampt::rec_type::CELL);
            }
        }
    }

    printLogLine(yampt::rec_type::CELL);
}

//----------------------------------------------------------
std::vector<std::tuple<std::string, size_t, bool>> DictCreator::makeDictCELLExtendedPattern()
{
    std::string pattern;
    std::vector<std::tuple<std::string, size_t, bool>> pattern_coll;
    for(size_t i = 0; i < esm_f.getRecordColl().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if(esm_f.getRecordId() == "CELL")
        {
            esm_f.setUniqueTo("NAME");
            if(esm_f.getUniqueStatus() == true)
            {
                // Pattern is the DATA and combined id of all objects in cell
                pattern.erase();
                esm_f.setFirstFriendlyTo("DATA", false);
                pattern += esm_f.getFriendlyText();
                esm_f.setFirstFriendlyTo("NAME", false); // Skipped
                while(esm_f.getFriendlyStatus() == true)
                {
                    esm_f.setNextFriendlyTo("NAME", false);
                    pattern += esm_f.getFriendlyText();
                }
                pattern_coll.push_back(make_tuple(pattern, i, false));
            }
        }
    }
    return pattern_coll;
}

//----------------------------------------------------------
std::map<std::string, size_t> DictCreator::makeDictCELLExtendedMatch()
{
    std::string match;
    std::map<std::string, size_t> match_coll;
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "CELL")
        {
            esm_n.setUniqueTo("NAME");
            if(esm_n.getUniqueStatus() == true)
            {
                match.erase();
                esm_n.setFirstFriendlyTo("DATA", false);
                match += esm_n.getFriendlyText();
                esm_n.setFirstFriendlyTo("NAME", false);
                while(esm_n.getFriendlyStatus() == true)
                {
                    esm_n.setNextFriendlyTo("NAME", false);
                    match += esm_n.getFriendlyText();
                }
                match_coll.insert({match, i});
            }
        }
    }
    return match_coll;
}

//----------------------------------------------------------
void DictCreator::makeDictCELLWilderness()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "GMST")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("STRV");
            esm_ptr->setRecordTo(i);
            esm_ptr->setUniqueTo("NAME");
            esm_ptr->setFirstFriendlyTo("STRV");
            if(esm_n.getUniqueText() == "sDefaultCellname" &&
               esm_n.getFriendlyStatus() == true &&
               esm_ptr->getUniqueText() == "sDefaultCellname" &&
               esm_ptr->getFriendlyStatus() == true)
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::CELL);
            }
        }
    }
    printLogLine(yampt::rec_type::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedWilderness()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "GMST")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("STRV");
            if(esm_n.getUniqueText() == "sDefaultCellname" &&
               esm_n.getFriendlyStatus() == true)
            {
                for(size_t k = 0; k < esm_f.getRecordColl().size(); ++k)
                {
                    esm_f.setRecordTo(k);
                    if(esm_f.getRecordId() == "GMST")
                    {
                        esm_f.setUniqueTo("NAME");
                        esm_f.setFirstFriendlyTo("STRV");
                        if(esm_f.getUniqueText() == "sDefaultCellname" &&
                           esm_f.getFriendlyStatus() == true)
                        {
                            validateRecord(esm_f.getFriendlyText(),
                                           esm_n.getFriendlyText(),
                                           yampt::rec_type::CELL);
                            break;
                        }
                    }
                }
            }
        }
    }
    printLogLine(yampt::rec_type::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLRegion()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "REGN")
        {
            //esm_n.setUnique("NAME");
            esm_n.setFirstFriendlyTo("FNAM");
            esm_ptr->setRecordTo(i);
            //esm_ptr->setUnique("NAME");
            esm_ptr->setFirstFriendlyTo("FNAM");
            if(//esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               //esm_ptr->getUniqueStatus() == true &&
               esm_ptr->getFriendlyStatus() == true)
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::CELL);
            }
        }
    }
    printLogLine(yampt::rec_type::Region);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedRegion()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "REGN")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("FNAM");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true)
            {
                for(size_t k = 0; k < esm_f.getRecordColl().size(); ++k)
                {
                    esm_f.setRecordTo(k);
                    if(esm_f.getRecordId() == "REGN")
                    {
                        esm_f.setUniqueTo("NAME");
                        esm_f.setFirstFriendlyTo("FNAM");
                        if(esm_f.getUniqueText() == esm_n.getUniqueText() &&
                           esm_f.getFriendlyStatus() == true)
                        {
                            validateRecord(esm_f.getFriendlyText(),
                                           esm_n.getFriendlyText(),
                                           yampt::rec_type::CELL);
                            break;
                        }
                    }
                }
            }
        }
    }
    printLogLine(yampt::rec_type::Region);
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "GMST")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("STRV");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               esm_n.getUniqueText().substr(0, 1) == "s") // Make sure is string
            {
                validateRecord(esm_n.getUniqueText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::GMST);
            }
        }
    }
    printLogLine(yampt::rec_type::GMST);
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "ACTI" ||
           esm_n.getRecordId() == "ALCH" ||
           esm_n.getRecordId() == "APPA" ||
           esm_n.getRecordId() == "ARMO" ||
           esm_n.getRecordId() == "BOOK" ||
           esm_n.getRecordId() == "BSGN" ||
           esm_n.getRecordId() == "CLAS" ||
           esm_n.getRecordId() == "CLOT" ||
           esm_n.getRecordId() == "CONT" ||
           esm_n.getRecordId() == "CREA" ||
           esm_n.getRecordId() == "DOOR" ||
           esm_n.getRecordId() == "FACT" ||
           esm_n.getRecordId() == "INGR" ||
           esm_n.getRecordId() == "LIGH" ||
           esm_n.getRecordId() == "LOCK" ||
           esm_n.getRecordId() == "MISC" ||
           esm_n.getRecordId() == "NPC_" ||
           esm_n.getRecordId() == "PROB" ||
           esm_n.getRecordId() == "RACE" ||
           esm_n.getRecordId() == "REGN" ||
           esm_n.getRecordId() == "REPA" ||
           esm_n.getRecordId() == "SKIL" ||
           esm_n.getRecordId() == "SPEL" ||
           esm_n.getRecordId() == "WEAP")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("FNAM");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               esm_n.getUniqueText() != "player") // Skip player name
            {
                validateRecord(esm_n.getRecordId() + yampt::sep[0] + esm_n.getUniqueText(),
                        esm_n.getFriendlyText(),
                        yampt::rec_type::FNAM);
            }
        }
    }
    printLogLine(yampt::rec_type::FNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "BSGN" ||
           esm_n.getRecordId() == "CLAS" ||
           esm_n.getRecordId() == "RACE")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("DESC");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true)
            {
                validateRecord(esm_n.getRecordId() + yampt::sep[0] + esm_n.getUniqueText(),
                        esm_n.getFriendlyText(),
                        yampt::rec_type::DESC);
            }
        }
    }
    printLogLine(yampt::rec_type::DESC);
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "BOOK")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("TEXT");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true)
            {
                validateRecord(esm_n.getUniqueText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::TEXT);
            }
        }
    }
    printLogLine(yampt::rec_type::TEXT);
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "FACT")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFirstFriendlyTo("RNAM");
            if(esm_n.getUniqueStatus() == true)
            {
                while(esm_n.getFriendlyStatus() == true)
                {
                    validateRecord(esm_n.getUniqueText() + yampt::sep[0] + std::to_string(esm_n.getFriendlyCounter()),
                            esm_n.getFriendlyText(),
                            yampt::rec_type::RNAM);

                    esm_n.setNextFriendlyTo("RNAM");
                }
            }
        }
    }
    printLogLine(yampt::rec_type::RNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "SKIL" ||
           esm_n.getRecordId() == "MGEF")
        {
            esm_n.setUniqueToINDX();
            esm_n.setFirstFriendlyTo("DESC");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true)
            {
                validateRecord(esm_n.getRecordId() + yampt::sep[0] + esm_n.getUniqueText(),
                        esm_n.getFriendlyText(),
                        yampt::rec_type::INDX);
            }
        }
    }
    printLogLine(yampt::rec_type::INDX);
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "DIAL")
        {
            esm_n.setUniqueToDialogType();
            esm_n.setFirstFriendlyTo("NAME");
            esm_ptr->setRecordTo(i);
            esm_ptr->setUniqueToDialogType();
            esm_ptr->setFirstFriendlyTo("NAME");
            if(esm_n.getUniqueText() == "T" &&
               esm_n.getFriendlyStatus() == true &&
               esm_ptr->getUniqueText() == "T" &&
               esm_ptr->getFriendlyStatus() == true)
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::DIAL);
            }
        }
    }
    printLogLine(yampt::rec_type::DIAL);
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtended()
{
    std::vector<std::tuple<std::string, size_t, bool>> pattern_coll;
    std::map<std::string, size_t> match_coll;

    pattern_coll = makeDictDIALExtendedPattern();
    match_coll = makeDictDIALExtendedMatch();

    resetCounters();
    for(size_t i = 0; i < pattern_coll.size(); ++i)
    {
        auto search = match_coll.find(std::get<0>(pattern_coll[i]));
        if(search != match_coll.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setFirstFriendlyTo("NAME");
            esm_f.setRecordTo(std::get<1>(pattern_coll[i]));
            esm_f.setFirstFriendlyTo("NAME");
            if(esm_n.getFriendlyStatus() == true &&
               esm_f.getFriendlyStatus() == true)
            {
                validateRecord(esm_f.getFriendlyText(),
                               esm_n.getFriendlyText(),
                               yampt::rec_type::DIAL);
            }
            std::get<2>(pattern_coll[i]) = true;
        }
        else
        {
            counter_missing++;
        }
    }

    for(size_t i = 0; i < pattern_coll.size(); ++i)
    {
        if(std::get<2>(pattern_coll[i]) == false)
        {
            esm_f.setRecordTo(std::get<1>(pattern_coll[i]));
            esm_f.setFirstFriendlyTo("NAME");
            if(esm_f.getFriendlyStatus() == true)
            {
                validateRecord(esm_f.getFriendlyText(),
                               yampt::err[0] + "MISSING" + yampt::err[1],
                        yampt::rec_type::DIAL);
            }
        }
    }

    printLogLine(yampt::rec_type::DIAL);
}

//----------------------------------------------------------
std::vector<std::tuple<std::string, size_t, bool>> DictCreator::makeDictDIALExtendedPattern()
{
    std::string pattern;
    std::vector<std::tuple<std::string, size_t, bool>> pattern_coll;
    for(size_t i = 0; i < esm_f.getRecordColl().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if(esm_f.getRecordId() == "DIAL")
        {
            esm_f.setUniqueToDialogType();
            if(esm_f.getUniqueText() == "T")
            {
                pattern.erase();
                esm_f.setRecordTo(i + 1);
                esm_f.setFirstFriendlyTo("INAM", false);
                pattern += esm_f.getFriendlyText();
                esm_f.setFirstFriendlyTo("SCVR", false);
                pattern += esm_f.getFriendlyText();
                pattern_coll.push_back(make_tuple(pattern, i, false));
            }
        }
    }
    return pattern_coll;
}

//----------------------------------------------------------
std::map<std::string, size_t> DictCreator::makeDictDIALExtendedMatch()
{
    std::string match;
    std::map<std::string, size_t> match_coll;
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "DIAL")
        {
            esm_n.setUniqueToDialogType();
            if(esm_n.getUniqueText() == "T")
            {
                match.erase();
                esm_n.setRecordTo(i + 1);
                esm_n.setFirstFriendlyTo("INAM", false);
                match += esm_n.getFriendlyText();
                esm_n.setFirstFriendlyTo("SCVR", false);
                match += esm_n.getFriendlyText();
                match_coll.insert({match, i});
            }
        }
    }
    return match_coll;
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
    std::string dialog_topic;

    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "DIAL")
        {
            esm_n.setUniqueToDialogType();
            esm_n.setFirstFriendlyTo("NAME");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true)
            {
                dialog_topic = esm_n.getUniqueText() + yampt::sep[0] + dialTranslator(esm_n.getFriendlyText());
            }
        }

        if(esm_n.getRecordId() == "INFO")
        {
            esm_n.setUniqueTo("INAM");
            esm_n.setFirstFriendlyTo("NAME");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true)
            {
                validateRecord(dialog_topic + yampt::sep[0] + esm_n.getUniqueText(),
                        esm_n.getFriendlyText(),
                        yampt::rec_type::INFO);
            }
        }
    }
    printLogLine(yampt::rec_type::INFO);
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
    std::string dialog_topic;

    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "INFO")
        {
            esm_n.setUniqueTo("INAM");
            esm_n.setFirstFriendlyTo("BNAM");
            esm_ptr->setRecordTo(i);
            esm_ptr->setUniqueTo("INAM");
            esm_ptr->setFirstFriendlyTo("BNAM");

            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               esm_ptr->getUniqueStatus() == true &&
               esm_ptr->getFriendlyStatus() == true)
            {
                message_n = makeMessageColl(esm_n.getFriendlyText());
                *message_ptr = makeMessageColl(esm_ptr->getFriendlyText());
                if(message_n.size() == message_ptr->size())
                {
                    for(size_t k = 0; k < message_n.size(); ++k)
                    {
                        validateRecord(yampt::sep[7] + esm_ptr->getUniqueText() + yampt::sep[8] + message_ptr->at(k),
                                yampt::sep[7] + esm_n.getUniqueText() + yampt::sep[8] + message_n.at(k),
                                yampt::rec_type::BNAM);
                    }
                }
                else
                {
                    // Missing?
                }
            }
        }
    }
    printLogLine(yampt::rec_type::BNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtended()
{
    std::vector<std::pair<std::string, size_t>> pattern_coll;
    std::map<std::string, size_t> match_coll;

    pattern_coll = makeDictBNAMExtendedPattern();
    match_coll = makeDictBNAMExtendedMatch();

    resetCounters();
    for(size_t i = 0; i < pattern_coll.size(); ++i)
    {
        auto search = match_coll.find(pattern_coll[i].first);
        if(search != match_coll.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setUniqueTo("INAM");
            esm_n.setFirstFriendlyTo("BNAM");
            esm_f.setRecordTo(pattern_coll[i].second);
            esm_f.setUniqueTo("INAM");
            esm_f.setFirstFriendlyTo("BNAM");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               esm_f.getUniqueStatus() == true &&
               esm_f.getFriendlyStatus() == true)
            {
                message_n = makeMessageColl(esm_n.getFriendlyText());
                message_f = makeMessageColl(esm_f.getFriendlyText());
                if(message_n.size() == message_f.size())
                {
                    for(size_t k = 0; k < message_n.size(); ++k)
                    {
                        validateRecord(yampt::sep[7] + esm_f.getUniqueText() + yampt::sep[8] + message_f.at(k),
                                yampt::sep[7] + esm_f.getUniqueText() + yampt::sep[8] + message_n.at(k),
                                yampt::rec_type::BNAM);
                    }
                }
            }
        }
    }
    printLogLine(yampt::rec_type::BNAM);
}

//----------------------------------------------------------
std::vector<std::pair<std::string, size_t>> DictCreator::makeDictBNAMExtendedPattern()
{
    std::string pattern;
    std::vector<std::pair<std::string, size_t>> pattern_coll;
    for(size_t i = 0; i < esm_f.getRecordColl().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if(esm_f.getRecordId() == "INFO")
        {
            esm_f.setUniqueTo("INAM");
            if(esm_f.getUniqueStatus() == true)
            {
                pattern = esm_f.getUniqueText();
                pattern_coll.push_back(make_pair(pattern, i));
            }
        }
    }
    return pattern_coll;
}

//----------------------------------------------------------
std::map<std::string, size_t> DictCreator::makeDictBNAMExtendedMatch()
{
    std::string match;
    std::map<std::string, size_t> match_coll;
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "INFO")
        {
            esm_n.setUniqueTo("INAM");
            if(esm_n.getUniqueStatus() == true)
            {
                match = esm_n.getUniqueText();
                match_coll.insert({match, i});
            }
        }
    }
    return match_coll;
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
    resetCounters();
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "SCPT")
        {
            esm_n.setUniqueTo("SCHD");
            esm_n.setFirstFriendlyTo("SCTX");
            esm_ptr->setRecordTo(i);
            esm_ptr->setUniqueTo("SCHD");
            esm_ptr->setFirstFriendlyTo("SCTX");

            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               esm_ptr->getUniqueStatus() == true &&
               esm_ptr->getFriendlyStatus() == true)
            {
                message_n = makeMessageColl(esm_n.getFriendlyText());
                *message_ptr = makeMessageColl(esm_ptr->getFriendlyText());
                if(message_n.size() == message_ptr->size())
                {
                    for(size_t k = 0; k < message_n.size(); ++k)
                    {
                        validateRecord(yampt::sep[7] + esm_ptr->getUniqueText() + yampt::sep[8] + message_ptr->at(k),
                                yampt::sep[7] + esm_n.getUniqueText() + yampt::sep[8] + message_n.at(k),
                                yampt::rec_type::SCTX);
                    }
                }
            }
        }
    }
    printLogLine(yampt::rec_type::SCTX);
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtended()
{
    std::vector<std::pair<std::string, size_t>> pattern_coll;
    std::map<std::string, size_t> match_coll;

    pattern_coll = makeDictSCPTExtendedPattern();
    match_coll = makeDictSCPTExtendedMatch();

    resetCounters();
    for(size_t i = 0; i < pattern_coll.size(); ++i)
    {
        auto search = match_coll.find(pattern_coll[i].first);
        if(search != match_coll.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setUniqueTo("SCHD");
            esm_n.setFirstFriendlyTo("SCTX");
            esm_f.setRecordTo(pattern_coll[i].second);
            esm_f.setUniqueTo("SCHD");
            esm_f.setFirstFriendlyTo("SCTX");
            if(esm_n.getUniqueStatus() == true &&
               esm_n.getFriendlyStatus() == true &&
               esm_f.getUniqueStatus() == true &&
               esm_f.getFriendlyStatus() == true)
            {
                message_n = makeMessageColl(esm_n.getFriendlyText());
                message_f = makeMessageColl(esm_f.getFriendlyText());
                if(message_n.size() == message_f.size())
                {
                    for(size_t k = 0; k < message_n.size(); ++k)
                    {
                        validateRecord(yampt::sep[7] + esm_f.getUniqueText() + yampt::sep[8] + message_f.at(k),
                                yampt::sep[7] + esm_n.getUniqueText() + yampt::sep[8] + message_n.at(k),
                                yampt::rec_type::SCTX);
                    }
                }
            }
        }
    }
    printLogLine(yampt::rec_type::SCTX);
}

//----------------------------------------------------------
std::vector<std::pair<std::string, size_t>> DictCreator::makeDictSCPTExtendedPattern()
{
    std::string pattern;
    std::vector<std::pair<std::string, size_t>> pattern_coll;
    for(size_t i = 0; i < esm_f.getRecordColl().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if(esm_f.getRecordId() == "SCPT")
        {
            esm_f.setUniqueTo("SCHD");
            if(esm_f.getUniqueStatus() == true)
            {
                pattern = esm_f.getUniqueText();
                pattern_coll.push_back(make_pair(pattern, i));
            }
        }
    }
    return pattern_coll;
}

//----------------------------------------------------------
std::map<std::string, size_t> DictCreator::makeDictSCPTExtendedMatch()
{
    std::string match;
    std::map<std::string, size_t> match_coll;
    for(size_t i = 0; i < esm_n.getRecordColl().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if(esm_n.getRecordId() == "SCPT")
        {
            esm_n.setUniqueTo("SCHD");
            if(esm_n.getUniqueStatus() == true)
            {
                match = esm_n.getUniqueText();
                match_coll.insert({match, i});
            }
        }
    }
    return match_coll;
}
