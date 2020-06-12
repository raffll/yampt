#include "dictcreator.hpp"

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & path_n
)
    : esm_n(path_n)
    , esm_ptr(&esm_n)
    , message_ptr(&message_n)
    , mode(Tools::CreatorMode::RAW)
    , add_hyperlinks(false)
{
    dict = Tools::initializeDict();

    if (esm_n.isLoaded())
        makeDict(true);
}

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & path_n,
    const std::string & path_f
)
    : esm_n(path_n)
    , esm_f(path_f)
    , esm_ptr(&esm_f)
    , message_ptr(&message_f)
    , mode(Tools::CreatorMode::BASE)
    , add_hyperlinks(false)
{
    dict = Tools::initializeDict();

    if (esm_n.isLoaded() &&
        esm_f.isLoaded())
    {
        makeDict(isSameOrder());
    }
}

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & path_n,
    const DictMerger & merger,
    const Tools::CreatorMode mode,
    const bool add_hyperlinks
)
    : esm_n(path_n)
    , esm_ptr(&esm_n)
    , merger(&merger)
    , message_ptr(&message_n)
    , mode(mode)
    , add_hyperlinks(add_hyperlinks)
{
    dict = Tools::initializeDict();

    if (esm_n.isLoaded())
        makeDict(true);
}

//----------------------------------------------------------
void DictCreator::makeDict(const bool same_order)
{
    Tools::addLog("-----------------------------------------------\r\n"
                  "          Created / Missing / Identical /   All\r\n"
                  "-----------------------------------------------\r\n");

    if (same_order)
    {
        makeDictCELL();
        makeDictCELLWilderness();
        makeDictCELLRegion();
        makeDictDIAL();
        makeDictBNAM();
        makeDictSCPT();
    }
    else
    {
        makeDictCELLExtended();
        makeDictCELLWildernessExtended();
        makeDictCELLRegionExtended();
        makeDictDIALExtended();
        makeDictBNAMExtended();
        makeDictSCPTExtended();
    }

    makeDictGMST();
    makeDictFNAM();
    makeDictDESC();
    makeDictTEXT();
    makeDictRNAM();
    makeDictINDX();

    if (add_hyperlinks)
    {
        Tools::addLog("Adding hyperlinks...\r\n");
    }

    makeDictINFO();

    if (!same_order)
        Tools::addLog("--> Check dictionary for \"MISSING\" keyword!\r\n"
                      "    Missing CELL and DIAL records needs to be added manually!\r\n");

    Tools::addLog("-----------------------------------------------\r\n");
}

//----------------------------------------------------------
void DictCreator::printLogLine(const Tools::RecType type)
{
    std::string id = Tools::getTypeName(type);
    id.resize(12, ' ');

    std::ostringstream ss;
    ss << id << std::setw(5) << std::to_string(counter_created) << " / ";

    if (type == Tools::RecType::CELL ||
        type == Tools::RecType::DIAL)
    {
        ss << std::setw(7) << std::to_string(counter_missing) << " / ";
    }
    else
    {
        ss << std::setw(7) << "-" << " / ";
    }

    ss << std::setw(8) << std::to_string(counter_identical) << " / ";
    ss << std::setw(6) << std::to_string(counter_all) << "\r\n";

    Tools::addLog(ss.str());
}

//----------------------------------------------------------
bool DictCreator::isSameOrder()
{
    std::string ids_n;
    std::string ids_f;

    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        ids_n += esm_n.getRecordId();
    }

    for (size_t i = 0; i < esm_f.getRecords().size(); ++i)
    {
        esm_f.setRecordTo(i);
        ids_f += esm_f.getRecordId();
    }

    return ids_n == ids_f;
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
void DictCreator::validateRecord(
    const std::string & unique_text,
    const std::string & friendly_text,
    const Tools::RecType type)
{
    counter_all++;

    // Insert without special cases
    if (mode == Tools::CreatorMode::RAW ||
        mode == Tools::CreatorMode::BASE)
    {
        insertRecordToDict(unique_text, friendly_text, type);
    }

    // For CELL, DIAL, BNAM, SCTX find corresponding record in dictionary given by user
    if (mode == Tools::CreatorMode::ALL)
    {
        validateRecordForModeALL(unique_text, friendly_text, type);
    }

    // Insert only ones not found in dictionary given by user
    if (mode == Tools::CreatorMode::NOTFOUND)
    {
        validateRecordForModeNOT(unique_text, friendly_text, type);
    }

    // Insert only with changed friendly text compared to dictionary given by user
    if (mode == Tools::CreatorMode::CHANGED)
    {
        validateRecordForModeCHANGED(unique_text, friendly_text, type);
    }
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeALL(
    const std::string & unique_text,
    const std::string & friendly_text,
    const Tools::RecType type)
{
    auto new_friendly = friendly_text;

    if (type == Tools::RecType::CELL ||
        type == Tools::RecType::DIAL ||
        type == Tools::RecType::BNAM ||
        type == Tools::RecType::SCTX)
    {
        auto search = merger->getDict().at(type).find(unique_text);
        if (search != merger->getDict().at(type).end())
        {
            new_friendly = search->second;
        }
    }

    insertRecordToDict(unique_text, new_friendly, type);
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeNOT(
    const std::string & unique_text,
    const std::string & friendly_text,
    const Tools::RecType type)
{
    auto new_friendly = friendly_text;

    auto search = merger->getDict().at(type).find(unique_text);
    if (search != merger->getDict().at(type).end())
        return;

    if (type == Tools::RecType::INFO && add_hyperlinks)
    {
        new_friendly = Tools::addHyperlinks(
            merger->getDict().at(Tools::RecType::DIAL),
            friendly_text,
            true);
    }

    insertRecordToDict(unique_text, new_friendly, type);
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeCHANGED(
    const std::string & unique_text,
    const std::string & friendly_text,
    const Tools::RecType type)
{
    auto new_friendly = friendly_text;

    if (type == Tools::RecType::CELL ||
        type == Tools::RecType::DIAL ||
        type == Tools::RecType::BNAM ||
        type == Tools::RecType::SCTX)
        return;

    auto search = merger->getDict().at(type).find(unique_text);
    if (search == merger->getDict().at(type).end())
        return;

    if (search->second == friendly_text)
        return;

    if (type == Tools::RecType::INFO && add_hyperlinks)
    {
        new_friendly = Tools::addHyperlinks(
            merger->getDict().at(Tools::RecType::DIAL),
            friendly_text,
            true);
    }

    insertRecordToDict(unique_text, new_friendly, type);
}

//----------------------------------------------------------
void DictCreator::insertRecordToDict(
    const std::string & unique_text,
    const std::string & friendly_text,
    const Tools::RecType type)
{
    if (dict[type].insert({ unique_text, friendly_text }).second)
    {
        counter_created++;
    }
    else
    {
        auto search = dict[type].find(unique_text);
        if (friendly_text != search->second)
        {
            auto new_unique =
                unique_text + Tools::err[0] + "DOUBLED_" +
                std::to_string(counter_doubled) + Tools::err[1];
            dict[type].insert({ new_unique, friendly_text });
            counter_doubled++;
            counter_created++;
            Tools::addLog("Doubled " + Tools::getTypeName(type) + ": " + unique_text + "\r\n");
        }
        else
        {
            counter_identical++;
        }
    }
}

//----------------------------------------------------------
std::string DictCreator::translateDialogTopicsInDictId(std::string to_translate)
{
    if (mode == Tools::CreatorMode::ALL ||
        mode == Tools::CreatorMode::NOTFOUND ||
        mode == Tools::CreatorMode::CHANGED)
    {
        auto search = merger->getDict().at(Tools::RecType::DIAL).find(to_translate);
        if (search != merger->getDict().at(Tools::RecType::DIAL).end())
        {
            return search->second;
        }
    }
    return to_translate;
}

//----------------------------------------------------------
std::vector<std::string> DictCreator::makeScriptMessages(const std::string & new_friendly)
{
    std::vector<std::string> messages;
    std::string line;
    std::string line_lc;
    std::istringstream ss(new_friendly);

    while (std::getline(ss, line))
    {
        line = Tools::trimCR(line);
        line_lc = line;
        transform(line_lc.begin(), line_lc.end(),
                  line_lc.begin(), ::tolower);

        size_t keyword_pos;
        std::set<size_t> keyword_pos_coll;

        for (const auto & keyword : Tools::keywords)
        {
            keyword_pos = line_lc.find(keyword);
            keyword_pos_coll.insert(keyword_pos);
        }

        keyword_pos = *keyword_pos_coll.begin();

        if (keyword_pos != std::string::npos &&
            line.rfind(";", keyword_pos) == std::string::npos &&
            line.find("\"", keyword_pos) != std::string::npos)
        {
            messages.push_back(line);
        }
    }
    return messages;
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "CELL")
        {
            esm_n.setFriendlyTo("NAME");
            esm_ptr->setRecordTo(i);
            esm_ptr->setFriendlyTo("NAME");
            if (esm_n.isFriendlyValid() &&
                esm_n.getFriendlyText() != "" &&
                esm_ptr->isFriendlyValid() &&
                esm_ptr->getFriendlyText() != "")
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::CELL);
            }
        }
    }
    printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLWilderness()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "GMST")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFriendlyTo("STRV");
            esm_ptr->setRecordTo(i);
            esm_ptr->setUniqueTo("NAME");
            esm_ptr->setFriendlyTo("STRV");
            if (esm_n.getUniqueText() == "sDefaultCellname" &&
                esm_n.isFriendlyValid() &&
                esm_ptr->getUniqueText() == "sDefaultCellname" &&
                esm_ptr->isFriendlyValid())
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::CELL);
            }
        }
    }
    printLogLine(Tools::RecType::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLWildernessExtended()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() != "GMST")
            continue;

        esm_n.setUniqueTo("NAME");
        esm_n.setFriendlyTo("STRV");
        if (esm_n.getUniqueText() == "sDefaultCellname" &&
            esm_n.isFriendlyValid())
        {
            for (size_t k = 0; k < esm_f.getRecords().size(); ++k)
            {
                esm_f.setRecordTo(k);
                if (esm_f.getRecordId() != "GMST")
                    continue;

                esm_f.setUniqueTo("NAME");
                esm_f.setFriendlyTo("STRV");
                if (esm_f.getUniqueText() == "sDefaultCellname" &&
                    esm_f.isFriendlyValid())
                {
                    validateRecord(esm_f.getFriendlyText(),
                                   esm_n.getFriendlyText(),
                                   Tools::RecType::CELL);
                    break;
                }
            }
            break;
        }
    }
    printLogLine(Tools::RecType::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLRegion()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "REGN")
        {
            esm_n.setFriendlyTo("FNAM");
            esm_ptr->setRecordTo(i);
            esm_ptr->setFriendlyTo("FNAM");
            if (esm_n.isFriendlyValid() &&
                esm_ptr->isFriendlyValid())
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::CELL);
            }
        }
    }
    printLogLine(Tools::RecType::Region);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLRegionExtended()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() != "REGN")
            continue;

        esm_n.setUniqueTo("NAME");
        esm_n.setFriendlyTo("FNAM");
        if (esm_n.isUniqueValid() &&
            esm_n.isFriendlyValid())
        {
            for (size_t k = 0; k < esm_f.getRecords().size(); ++k)
            {
                esm_f.setRecordTo(k);
                if (esm_f.getRecordId() != "REGN")
                    continue;

                esm_f.setUniqueTo("NAME");
                esm_f.setFriendlyTo("FNAM");
                if (esm_f.getUniqueText() == esm_n.getUniqueText() &&
                    esm_f.isFriendlyValid())
                {
                    validateRecord(esm_f.getFriendlyText(),
                                   esm_n.getFriendlyText(),
                                   Tools::RecType::CELL);
                    break;
                }
            }
        }
    }
    printLogLine(Tools::RecType::Region);
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "GMST")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFriendlyTo("STRV");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid() &&
                esm_n.getUniqueText().substr(0, 1) == "s")
            {
                validateRecord(esm_n.getUniqueText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::GMST);
            }
        }
    }
    printLogLine(Tools::RecType::GMST);
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "ACTI" || esm_n.getRecordId() == "ALCH" ||
            esm_n.getRecordId() == "APPA" || esm_n.getRecordId() == "ARMO" ||
            esm_n.getRecordId() == "BOOK" || esm_n.getRecordId() == "BSGN" ||
            esm_n.getRecordId() == "CLAS" || esm_n.getRecordId() == "CLOT" ||
            esm_n.getRecordId() == "CONT" || esm_n.getRecordId() == "CREA" ||
            esm_n.getRecordId() == "DOOR" || esm_n.getRecordId() == "FACT" ||
            esm_n.getRecordId() == "INGR" || esm_n.getRecordId() == "LIGH" ||
            esm_n.getRecordId() == "LOCK" || esm_n.getRecordId() == "MISC" ||
            esm_n.getRecordId() == "NPC_" || esm_n.getRecordId() == "PROB" ||
            esm_n.getRecordId() == "RACE" || esm_n.getRecordId() == "REGN" ||
            esm_n.getRecordId() == "REPA" || esm_n.getRecordId() == "SKIL" ||
            esm_n.getRecordId() == "SPEL" || esm_n.getRecordId() == "WEAP")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFriendlyTo("FNAM");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid() &&
                esm_n.getUniqueText() != "player")
            {
                validateRecord(
                    esm_n.getRecordId() + Tools::sep[0] + esm_n.getUniqueText(),
                    esm_n.getFriendlyText(),
                    Tools::RecType::FNAM);
            }
        }
    }
    printLogLine(Tools::RecType::FNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "BSGN" ||
            esm_n.getRecordId() == "CLAS" ||
            esm_n.getRecordId() == "RACE")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFriendlyTo("DESC");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid())
            {
                validateRecord(
                    esm_n.getRecordId() + Tools::sep[0] + esm_n.getUniqueText(),
                    esm_n.getFriendlyText(),
                    Tools::RecType::DESC);
            }
        }
    }
    printLogLine(Tools::RecType::DESC);
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "BOOK")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFriendlyTo("TEXT");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid())
            {
                validateRecord(esm_n.getUniqueText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::TEXT);
            }
        }
    }
    printLogLine(Tools::RecType::TEXT);
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "FACT")
        {
            esm_n.setUniqueTo("NAME");
            esm_n.setFriendlyTo("RNAM");
            if (esm_n.isUniqueValid())
            {
                while (esm_n.isFriendlyValid())
                {
                    validateRecord(
                        esm_n.getUniqueText() + Tools::sep[0] + esm_n.getFriendlyCounter(),
                        esm_n.getFriendlyText(),
                        Tools::RecType::RNAM);

                    esm_n.setNextFriendlyTo("RNAM");
                }
            }
        }
    }
    printLogLine(Tools::RecType::RNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "SKIL" ||
            esm_n.getRecordId() == "MGEF")
        {
            esm_n.setUniqueTo("INDX");
            esm_n.setFriendlyTo("DESC");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid())
            {
                validateRecord(
                    esm_n.getRecordId() + Tools::sep[0] + esm_n.getUniqueText(),
                    esm_n.getFriendlyText(),
                    Tools::RecType::INDX);
            }
        }
    }
    printLogLine(Tools::RecType::INDX);
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "DIAL")
        {
            esm_n.setUniqueTo("DATA");
            esm_n.setFriendlyTo("NAME");
            esm_ptr->setRecordTo(i);
            esm_ptr->setUniqueTo("DATA");
            esm_ptr->setFriendlyTo("NAME");
            if (esm_n.getUniqueText() == "T" &&
                esm_n.isFriendlyValid() &&
                esm_ptr->getUniqueText() == "T" &&
                esm_ptr->isFriendlyValid())
            {
                validateRecord(esm_ptr->getFriendlyText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::DIAL);
            }
        }
    }
    printLogLine(Tools::RecType::DIAL);
}


//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
    std::string dialog_topic;
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "DIAL")
        {
            esm_n.setUniqueTo("DATA");
            esm_n.setFriendlyTo("NAME");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid())
            {
                dialog_topic = esm_n.getUniqueText() + Tools::sep[0] +
                    translateDialogTopicsInDictId(esm_n.getFriendlyText());
            }
        }
        if (esm_n.getRecordId() == "INFO")
        {
            esm_n.setUniqueTo("INAM");
            esm_n.setFriendlyTo("NAME");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid())
            {
                validateRecord(
                    dialog_topic + Tools::sep[0] + esm_n.getUniqueText(),
                    esm_n.getFriendlyText(),
                    Tools::RecType::INFO);
            }
        }
    }
    printLogLine(Tools::RecType::INFO);
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() != "INFO")
            continue;

        esm_n.setUniqueTo("INAM");
        esm_n.setFriendlyTo("BNAM");
        esm_ptr->setRecordTo(i);
        esm_ptr->setUniqueTo("INAM");
        esm_ptr->setFriendlyTo("BNAM");

        if (esm_n.isUniqueValid() &&
            esm_n.isFriendlyValid() &&
            esm_ptr->isUniqueValid() &&
            esm_ptr->isFriendlyValid())
        {
            message_n = makeScriptMessages(esm_n.getFriendlyText());
            *message_ptr = makeScriptMessages(esm_ptr->getFriendlyText());
            if (message_n.size() == message_ptr->size())
            {
                for (size_t k = 0; k < message_n.size(); ++k)
                {
                    validateRecord(
                        esm_ptr->getUniqueText() + Tools::sep[0] + message_ptr->at(k),
                        esm_n.getUniqueText() + Tools::sep[0] + message_n.at(k),
                        Tools::RecType::BNAM);
                }
            }
        }
    }
    printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() != "SCPT")
            continue;

        esm_n.setUniqueTo("SCHD");
        esm_n.setFriendlyTo("SCTX");
        esm_ptr->setRecordTo(i);
        esm_ptr->setUniqueTo("SCHD");
        esm_ptr->setFriendlyTo("SCTX");

        if (esm_n.isUniqueValid() &&
            esm_n.isFriendlyValid() &&
            esm_ptr->isUniqueValid() &&
            esm_ptr->isFriendlyValid())
        {
            message_n = makeScriptMessages(esm_n.getFriendlyText());
            *message_ptr = makeScriptMessages(esm_ptr->getFriendlyText());
            if (message_n.size() == message_ptr->size())
            {
                for (size_t k = 0; k < message_n.size(); ++k)
                {
                    validateRecord(
                        esm_ptr->getUniqueText() + Tools::sep[0] + message_ptr->at(k),
                        esm_n.getUniqueText() + Tools::sep[0] + message_n.at(k),
                        Tools::RecType::SCTX);
                }
            }
        }
    }
    printLogLine(Tools::RecType::SCTX);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtended()
{
    makeDictCELLExtendedForeignColl();
    makeDictCELLExtendedNativeColl();

    resetCounters();
    for (size_t i = 0; i < patterns_f.size(); ++i)
    {
        auto search = patterns_n.find(patterns_f[i].str);
        if (search != patterns_n.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setFriendlyTo("NAME");
            esm_f.setRecordTo(patterns_f[i].pos);
            esm_f.setFriendlyTo("NAME");
            if (esm_n.isFriendlyValid() &&
                esm_n.getFriendlyText() != "" &&
                esm_f.isFriendlyValid() &&
                esm_f.getFriendlyText() != "")
            {
                validateRecord(esm_f.getFriendlyText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::CELL);
            }
        }
        else
        {
            patterns_f[i].missing = true;
            counter_missing++;
        }
    }
    makeDictCELLExtendedAddMissing();
    printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedForeignColl()
{
    patterns_f.clear();
    for (size_t i = 0; i < esm_f.getRecords().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if (esm_f.getRecordId() == "CELL")
        {
            esm_f.setFriendlyTo("NAME");
            if (esm_f.isFriendlyValid() &&
                esm_f.getFriendlyText() != "")
            {
                patterns_f.push_back({ makeDictCELLExtendedPattern(esm_f), i, false });
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedNativeColl()
{
    patterns_n.clear();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "CELL")
        {
            esm_n.setFriendlyTo("NAME");
            if (esm_n.isFriendlyValid() &&
                esm_n.getFriendlyText() != "")
            {
                patterns_n.insert({ makeDictCELLExtendedPattern(esm_n), i });
            }
        }
    }
}

//----------------------------------------------------------
std::string DictCreator::makeDictCELLExtendedPattern(EsmReader & esm_cur)
{
    // Pattern is the DATA and combined ids of all objects in cell

    std::string pattern;
    esm_cur.setFriendlyTo("DATA");
    pattern += esm_cur.getFriendlyWithNull();
    esm_cur.setFriendlyTo("NAME");
    while (esm_cur.isFriendlyValid())
    {
        esm_cur.setNextFriendlyTo("NAME");
        pattern += esm_cur.getFriendlyWithNull();
    }
    return pattern;
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedAddMissing()
{
    for (size_t i = 0; i < patterns_f.size(); ++i)
    {
        if (patterns_f[i].missing)
        {
            esm_f.setRecordTo(patterns_f[i].pos);
            esm_f.setFriendlyTo("NAME");
            if (esm_f.isFriendlyValid() &&
                esm_f.getFriendlyText() != "")
            {
                validateRecord(esm_f.getFriendlyText(),
                               Tools::err[0] + "MISSING" + Tools::err[1],
                               Tools::RecType::CELL);
                Tools::addLog("Missing CELL: " + esm_f.getFriendlyText() + "\r\n");
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtended()
{
    makeDictDIALExtendedForeignColl();
    makeDictDIALExtendedNativeColl();

    resetCounters();
    for (size_t i = 0; i < patterns_f.size(); ++i)
    {
        auto search = patterns_n.find(patterns_f[i].str);
        if (search != patterns_n.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setFriendlyTo("NAME");
            esm_f.setRecordTo(patterns_f[i].pos);
            esm_f.setFriendlyTo("NAME");
            if (esm_n.isFriendlyValid() &&
                esm_f.isFriendlyValid())
            {
                validateRecord(esm_f.getFriendlyText(),
                               esm_n.getFriendlyText(),
                               Tools::RecType::DIAL);
            }
        }
        else
        {
            patterns_f[i].missing = true;
            counter_missing++;
        }
    }
    makeDictDIALExtendedAddMissing();
    printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtendedForeignColl()
{
    patterns_f.clear();
    for (size_t i = 0; i < esm_f.getRecords().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if (esm_f.getRecordId() == "DIAL")
        {
            esm_f.setUniqueTo("DATA");
            if (esm_f.getUniqueText() == "T")
            {
                patterns_f.push_back({ makeDictDIALExtendedPattern(esm_f, i), i, false });
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtendedNativeColl()
{
    patterns_n.clear();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "DIAL")
        {
            esm_n.setUniqueTo("DATA");
            if (esm_n.getUniqueText() == "T")
            {
                patterns_n.insert({ makeDictDIALExtendedPattern(esm_n, i), i });
            }
        }
    }
}

//----------------------------------------------------------
std::string DictCreator::makeDictDIALExtendedPattern(EsmReader & esm_cur, size_t i)
{
    // Pattern is the INAM and SCVR from next INFO record

    std::string pattern;
    esm_cur.setRecordTo(i + 1);
    esm_cur.setFriendlyTo("INAM");
    pattern += esm_cur.getFriendlyWithNull();
    esm_cur.setFriendlyTo("SCVR");
    pattern += esm_cur.getFriendlyWithNull();
    return pattern;
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtendedAddMissing()
{
    for (size_t i = 0; i < patterns_f.size(); ++i)
    {
        if (patterns_f[i].missing)
        {
            esm_f.setRecordTo(patterns_f[i].pos);
            esm_f.setFriendlyTo("NAME");
            if (esm_f.isFriendlyValid())
            {
                validateRecord(esm_f.getFriendlyText(),
                               Tools::err[0] + "MISSING" + Tools::err[1],
                               Tools::RecType::DIAL);
                Tools::addLog("Missing DIAL: " + esm_f.getFriendlyText() + "\r\n");
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtended()
{
    makeDictBNAMExtendedForeignColl();
    makeDictBNAMExtendedNativeColl();

    resetCounters();
    for (size_t i = 0; i < patterns_f.size(); ++i)
    {
        auto search = patterns_n.find(patterns_f[i].str);
        if (search != patterns_n.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setUniqueTo("INAM");
            esm_n.setFriendlyTo("BNAM");
            esm_f.setRecordTo(patterns_f[i].pos);
            esm_f.setUniqueTo("INAM");
            esm_f.setFriendlyTo("BNAM");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid() &&
                esm_f.isUniqueValid() &&
                esm_f.isFriendlyValid())
            {
                message_n = makeScriptMessages(esm_n.getFriendlyText());
                message_f = makeScriptMessages(esm_f.getFriendlyText());
                if (message_n.size() == message_f.size())
                {
                    for (size_t k = 0; k < message_n.size(); ++k)
                    {
                        validateRecord(
                            esm_f.getUniqueText() + Tools::sep[0] + message_f.at(k),
                            esm_f.getUniqueText() + Tools::sep[0] + message_n.at(k),
                            Tools::RecType::BNAM);
                    }
                }
            }
        }
    }
    printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtendedForeignColl()
{
    patterns_f.clear();
    for (size_t i = 0; i < esm_f.getRecords().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if (esm_f.getRecordId() == "INFO")
        {
            esm_f.setUniqueTo("INAM");
            if (esm_f.isUniqueValid())
            {
                patterns_f.push_back({ esm_f.getUniqueText(), i, false });
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtendedNativeColl()
{
    patterns_n.clear();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "INFO")
        {
            esm_n.setUniqueTo("INAM");
            if (esm_n.isUniqueValid())
            {
                patterns_n.insert({ esm_n.getUniqueText(), i });
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtended()
{
    makeDictSCPTExtendedForeignColl();
    makeDictSCPTExtendedNativeColl();

    resetCounters();
    for (size_t i = 0; i < patterns_f.size(); ++i)
    {
        auto search = patterns_n.find(patterns_f[i].str);
        if (search != patterns_n.end())
        {
            esm_n.setRecordTo(search->second);
            esm_n.setUniqueTo("SCHD");
            esm_n.setFriendlyTo("SCTX");
            esm_f.setRecordTo(patterns_f[i].pos);
            esm_f.setUniqueTo("SCHD");
            esm_f.setFriendlyTo("SCTX");
            if (esm_n.isUniqueValid() &&
                esm_n.isFriendlyValid() &&
                esm_f.isUniqueValid() &&
                esm_f.isFriendlyValid())
            {
                message_n = makeScriptMessages(esm_n.getFriendlyText());
                message_f = makeScriptMessages(esm_f.getFriendlyText());
                if (message_n.size() == message_f.size())
                {
                    for (size_t k = 0; k < message_n.size(); ++k)
                    {
                        validateRecord(
                            esm_f.getUniqueText() + Tools::sep[0] + message_f.at(k),
                            esm_n.getUniqueText() + Tools::sep[0] + message_n.at(k),
                            Tools::RecType::SCTX);
                    }
                }
            }
        }
    }
    printLogLine(Tools::RecType::SCTX);
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtendedForeignColl()
{
    patterns_f.clear();
    for (size_t i = 0; i < esm_f.getRecords().size(); ++i)
    {
        esm_f.setRecordTo(i);
        if (esm_f.getRecordId() == "SCPT")
        {
            esm_f.setUniqueTo("SCHD");
            if (esm_f.isUniqueValid())
            {
                patterns_f.push_back({ esm_f.getUniqueText(), i , false});
            }
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtendedNativeColl()
{
    patterns_n.clear();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.setRecordTo(i);
        if (esm_n.getRecordId() == "SCPT")
        {
            esm_n.setUniqueTo("SCHD");
            if (esm_n.isUniqueValid())
            {
                patterns_n.insert({ esm_n.getUniqueText(), i });
            }
        }
    }
}
