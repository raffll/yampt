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
bool DictCreator::isSameOrder()
{
    std::string ids_n;
    std::string ids_f;

    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.selectRecord(i);
        ids_n += esm_n.getRecordId();
    }

    for (size_t i = 0; i < esm_f.getRecords().size(); ++i)
    {
        esm_f.selectRecord(i);
        ids_f += esm_f.getRecordId();
    }

    return ids_n == ids_f;
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
    resetCounters();
    for (size_t i = 0; i < esm_n.getRecords().size(); ++i)
    {
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "CELL")
            continue;

        esm_n.setValue("NAME");
        esm_ptr->selectRecord(i);
        esm_ptr->setValue("NAME");

        if (esm_n.getValue().exist &&
            esm_n.getValue().text != "" &&
            esm_ptr->getValue().exist &&
            esm_ptr->getValue().text != "")
        {
            key_text = esm_ptr->getValue().text;
            val_text = esm_n.getValue().text;
            type = Tools::RecType::CELL;
            validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "GMST")
            continue;

        esm_n.setKey("NAME");
        esm_n.setValue("STRV");
        esm_ptr->selectRecord(i);
        esm_ptr->setKey("NAME");
        esm_ptr->setValue("STRV");

        if (esm_n.getKey().text == "sDefaultCellname" &&
            esm_n.getValue().exist &&
            esm_ptr->getKey().text == "sDefaultCellname" &&
            esm_ptr->getValue().exist)
        {
            key_text = esm_ptr->getValue().text;
            val_text = esm_n.getValue().text;
            type = Tools::RecType::CELL;
            validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "GMST")
            continue;

        esm_n.setKey("NAME");
        esm_n.setValue("STRV");

        if (esm_n.getKey().text == "sDefaultCellname" &&
            esm_n.getValue().exist)
        {
            for (size_t k = 0; k < esm_f.getRecords().size(); ++k)
            {
                esm_f.selectRecord(k);
                if (esm_f.getRecordId() != "GMST")
                    continue;

                esm_f.setKey("NAME");
                esm_f.setValue("STRV");

                if (esm_f.getKey().text == "sDefaultCellname" &&
                    esm_f.getValue().exist)
                {
                    key_text = esm_f.getValue().text;
                    val_text = esm_n.getValue().text;
                    type = Tools::RecType::CELL;
                    validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "REGN")
            continue;

        esm_n.setValue("FNAM");
        esm_ptr->selectRecord(i);
        esm_ptr->setValue("FNAM");

        if (esm_n.getValue().exist &&
            esm_ptr->getValue().exist)
        {
            key_text = esm_ptr->getValue().text;
            val_text = esm_n.getValue().text;
            type = Tools::RecType::CELL;
            validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "REGN")
            continue;

        esm_n.setKey("NAME");
        esm_n.setValue("FNAM");

        if (esm_n.getKey().exist &&
            esm_n.getValue().exist)
        {
            for (size_t k = 0; k < esm_f.getRecords().size(); ++k)
            {
                esm_f.selectRecord(k);
                if (esm_f.getRecordId() != "REGN")
                    continue;

                esm_f.setKey("NAME");
                esm_f.setValue("FNAM");

                if (esm_f.getKey().text == esm_n.getKey().text &&
                    esm_f.getValue().exist)
                {
                    key_text = esm_f.getValue().text;
                    val_text = esm_n.getValue().text;
                    type = Tools::RecType::CELL;
                    validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "GMST")
            continue;

        esm_n.setKey("NAME");
        esm_n.setValue("STRV");

        if (esm_n.getKey().exist &&
            esm_n.getValue().exist &&
            esm_n.getKey().text.substr(0, 1) == "s")
        {
            key_text = esm_n.getKey().text;
            val_text = esm_n.getValue().text;
            type = Tools::RecType::GMST;
            validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "ACTI" ||
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
            esm_n.setKey("NAME");
            esm_n.setValue("FNAM");

            if (esm_n.getKey().exist &&
                esm_n.getValue().exist &&
                esm_n.getKey().text != "player")
            {
                key_text = esm_n.getRecordId() + Tools::sep[0] + esm_n.getKey().text;
                val_text = esm_n.getValue().text;
                type = Tools::RecType::FNAM;
                validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "BSGN" ||
            esm_n.getRecordId() == "CLAS" ||
            esm_n.getRecordId() == "RACE")
        {
            esm_n.setKey("NAME");
            esm_n.setValue("DESC");

            if (esm_n.getKey().exist &&
                esm_n.getValue().exist)
            {
                key_text = esm_n.getRecordId() + Tools::sep[0] + esm_n.getKey().text;
                val_text = esm_n.getValue().text;
                type = Tools::RecType::DESC;
                validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "BOOK")
            continue;

        esm_n.setKey("NAME");
        esm_n.setValue("TEXT");

        if (esm_n.getKey().exist &&
            esm_n.getValue().exist)
        {
            key_text = esm_n.getKey().text;
            val_text = esm_n.getValue().text;
            type = Tools::RecType::TEXT;
            validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "FACT")
            continue;

        esm_n.setKey("NAME");
        esm_n.setValue("RNAM");

        if (!esm_n.getKey().exist)
            continue;

        while (esm_n.getValue().exist)
        {
            key_text = esm_n.getKey().text + Tools::sep[0] + std::to_string(esm_n.getValue().counter);
            val_text = esm_n.getValue().text;
            type = Tools::RecType::RNAM;
            validateRecord();
            esm_n.setNextValue("RNAM");
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "SKIL" ||
            esm_n.getRecordId() == "MGEF")
        {
            esm_n.setKey("INDX");
            esm_n.setValue("DESC");

            if (esm_n.getKey().exist &&
                esm_n.getValue().exist)
            {
                key_text = esm_n.getRecordId() + Tools::sep[0] + esm_n.getKey().text;
                val_text = esm_n.getValue().text;
                type = Tools::RecType::INDX;
                validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "DIAL")
            continue;

        esm_n.setKey("DATA");
        esm_n.setValue("NAME");
        esm_ptr->selectRecord(i);
        esm_ptr->setKey("DATA");
        esm_ptr->setValue("NAME");

        if (esm_n.getKey().text == "T" &&
            esm_n.getValue().exist &&
            esm_ptr->getKey().text == "T" &&
            esm_ptr->getValue().exist)
        {
            key_text = esm_ptr->getValue().text;
            val_text = esm_n.getValue().text;
            type = Tools::RecType::DIAL;
            validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "DIAL")
        {
            esm_n.setKey("DATA");
            esm_n.setValue("NAME");

            if (esm_n.getKey().exist &&
                esm_n.getValue().exist)
            {
                dialog_topic = esm_n.getKey().text + Tools::sep[0] +
                    translateDialogTopicsInDictId(esm_n.getValue().text);
            }
        }
        if (esm_n.getRecordId() == "INFO")
        {
            esm_n.setKey("INAM");
            esm_n.setValue("NAME");

            if (esm_n.getKey().exist &&
                esm_n.getValue().exist)
            {
                key_text = dialog_topic + Tools::sep[0] + esm_n.getKey().text;
                val_text = esm_n.getValue().text;
                type = Tools::RecType::INFO;
                validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "INFO")
            continue;

        esm_n.setKey("INAM");
        esm_n.setValue("BNAM");
        esm_ptr->selectRecord(i);
        esm_ptr->setKey("INAM");
        esm_ptr->setValue("BNAM");

        if (esm_n.getKey().exist &&
            esm_n.getValue().exist &&
            esm_ptr->getKey().exist &&
            esm_ptr->getValue().exist)
        {
            message_n = makeScriptMessages(esm_n.getValue().text);
            *message_ptr = makeScriptMessages(esm_ptr->getValue().text);

            if (message_n.size() != message_ptr->size())
                continue;

            for (size_t k = 0; k < message_n.size(); ++k)
            {
                key_text = esm_ptr->getKey().text + Tools::sep[0] + message_ptr->at(k);
                val_text = esm_n.getKey().text + Tools::sep[0] + message_n.at(k);
                type = Tools::RecType::BNAM;
                validateRecord();
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() != "SCPT")
            continue;

        esm_n.setKey("SCHD");
        esm_n.setValue("SCTX");
        esm_ptr->selectRecord(i);
        esm_ptr->setKey("SCHD");
        esm_ptr->setValue("SCTX");

        if (esm_n.getKey().exist &&
            esm_n.getValue().exist &&
            esm_ptr->getKey().exist &&
            esm_ptr->getValue().exist)
        {
            message_n = makeScriptMessages(esm_n.getValue().text);
            *message_ptr = makeScriptMessages(esm_ptr->getValue().text);
            if (message_n.size() == message_ptr->size())
            {
                for (size_t k = 0; k < message_n.size(); ++k)
                {
                    key_text = esm_ptr->getKey().text + Tools::sep[0] + message_ptr->at(k);
                    val_text = esm_n.getKey().text + Tools::sep[0] + message_n.at(k);
                    type = Tools::RecType::SCTX;
                    validateRecord();
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
            esm_n.selectRecord(search->second);
            esm_n.setValue("NAME");
            esm_f.selectRecord(patterns_f[i].pos);
            esm_f.setValue("NAME");

            if (esm_n.getValue().exist &&
                esm_n.getValue().text != "" &&
                esm_f.getValue().exist &&
                esm_f.getValue().text != "")
            {
                key_text = esm_f.getValue().text;
                val_text = esm_n.getValue().text;
                type = Tools::RecType::CELL;
                validateRecord();
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
        esm_f.selectRecord(i);
        if (esm_f.getRecordId() == "CELL")
        {
            esm_f.setValue("NAME");
            if (esm_f.getValue().exist &&
                esm_f.getValue().text != "")
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "CELL")
        {
            esm_n.setValue("NAME");
            if (esm_n.getValue().exist &&
                esm_n.getValue().text != "")
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
    esm_cur.setValue("DATA");
    pattern += esm_cur.getValue().content;
    esm_cur.setValue("NAME");
    while (esm_cur.getValue().exist)
    {
        esm_cur.setNextValue("NAME");
        pattern += esm_cur.getValue().content;
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
            esm_f.selectRecord(patterns_f[i].pos);
            esm_f.setValue("NAME");

            if (esm_f.getValue().exist &&
                esm_f.getValue().text != "")
            {
                key_text = esm_f.getValue().text;
                val_text = Tools::err[0] + "MISSING" + Tools::err[1];
                type = Tools::RecType::CELL;
                validateRecord();
                Tools::addLog("Missing CELL: " + esm_f.getValue().text + "\r\n");
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
            esm_n.selectRecord(search->second);
            esm_n.setValue("NAME");
            esm_f.selectRecord(patterns_f[i].pos);
            esm_f.setValue("NAME");

            if (esm_n.getValue().exist &&
                esm_f.getValue().exist)
            {
                key_text = esm_f.getValue().text;
                val_text = esm_n.getValue().text;
                type = Tools::RecType::DIAL;
                validateRecord();
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
        esm_f.selectRecord(i);
        if (esm_f.getRecordId() == "DIAL")
        {
            esm_f.setKey("DATA");
            if (esm_f.getKey().text == "T")
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "DIAL")
        {
            esm_n.setKey("DATA");
            if (esm_n.getKey().text == "T")
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
    esm_cur.selectRecord(i + 1);
    esm_cur.setValue("INAM");
    pattern += esm_cur.getValue().content;
    esm_cur.setValue("SCVR");
    pattern += esm_cur.getValue().content;
    return pattern;
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtendedAddMissing()
{
    for (size_t i = 0; i < patterns_f.size(); ++i)
    {
        if (patterns_f[i].missing)
        {
            esm_f.selectRecord(patterns_f[i].pos);
            esm_f.setValue("NAME");

            if (esm_f.getValue().exist)
            {
                key_text = esm_f.getValue().text;
                val_text = Tools::err[0] + "MISSING" + Tools::err[1];
                type = Tools::RecType::DIAL;
                validateRecord();
                Tools::addLog("Missing DIAL: " + esm_f.getValue().text + "\r\n");
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
            esm_n.selectRecord(search->second);
            esm_n.setKey("INAM");
            esm_n.setValue("BNAM");
            esm_f.selectRecord(patterns_f[i].pos);
            esm_f.setKey("INAM");
            esm_f.setValue("BNAM");

            if (esm_n.getKey().exist &&
                esm_n.getValue().exist &&
                esm_f.getKey().exist &&
                esm_f.getValue().exist)
            {
                message_n = makeScriptMessages(esm_n.getValue().text);
                message_f = makeScriptMessages(esm_f.getValue().text);
                if (message_n.size() == message_f.size())
                {
                    for (size_t k = 0; k < message_n.size(); ++k)
                    {
                        key_text = esm_f.getKey().text + Tools::sep[0] + message_f.at(k);
                        val_text = esm_f.getKey().text + Tools::sep[0] + message_n.at(k);
                        type = Tools::RecType::BNAM;
                        validateRecord();
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
        esm_f.selectRecord(i);
        if (esm_f.getRecordId() == "INFO")
        {
            esm_f.setKey("INAM");
            if (esm_f.getKey().exist)
            {
                patterns_f.push_back({ esm_f.getKey().text, i, false });
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "INFO")
        {
            esm_n.setKey("INAM");
            if (esm_n.getKey().exist)
            {
                patterns_n.insert({ esm_n.getKey().text, i });
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
            esm_n.selectRecord(search->second);
            esm_n.setKey("SCHD");
            esm_n.setValue("SCTX");
            esm_f.selectRecord(patterns_f[i].pos);
            esm_f.setKey("SCHD");
            esm_f.setValue("SCTX");

            if (esm_n.getKey().exist &&
                esm_n.getValue().exist &&
                esm_f.getKey().exist &&
                esm_f.getValue().exist)
            {
                message_n = makeScriptMessages(esm_n.getValue().text);
                message_f = makeScriptMessages(esm_f.getValue().text);
                if (message_n.size() == message_f.size())
                {
                    for (size_t k = 0; k < message_n.size(); ++k)
                    {
                        key_text = esm_f.getKey().text + Tools::sep[0] + message_f.at(k);
                        val_text = esm_n.getKey().text + Tools::sep[0] + message_n.at(k);
                        type = Tools::RecType::SCTX;
                        validateRecord();
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
        esm_f.selectRecord(i);
        if (esm_f.getRecordId() == "SCPT")
        {
            esm_f.setKey("SCHD");
            if (esm_f.getKey().exist)
            {
                patterns_f.push_back({ esm_f.getKey().text, i , false });
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
        esm_n.selectRecord(i);
        if (esm_n.getRecordId() == "SCPT")
        {
            esm_n.setKey("SCHD");
            if (esm_n.getKey().exist)
            {
                patterns_n.insert({ esm_n.getKey().text, i });
            }
        }
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
void DictCreator::validateRecord()
{
    counter_all++;

    if (mode == Tools::CreatorMode::RAW ||
        mode == Tools::CreatorMode::BASE)
    {
        insertRecordToDict();
    }

    if (mode == Tools::CreatorMode::ALL)
    {
        validateRecordForModeALL();
    }

    if (mode == Tools::CreatorMode::NOTFOUND)
    {
        validateRecordForModeNOT();
    }

    if (mode == Tools::CreatorMode::CHANGED)
    {
        validateRecordForModeCHANGED();
    }
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeALL()
{
    if (type == Tools::RecType::CELL ||
        type == Tools::RecType::DIAL ||
        type == Tools::RecType::BNAM ||
        type == Tools::RecType::SCTX)
    {
        auto search = merger->getDict().at(type).find(key_text);
        if (search != merger->getDict().at(type).end())
        {
            val_text = search->second;
        }
    }

    insertRecordToDict();
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeNOT()
{
    auto search = merger->getDict().at(type).find(key_text);
    if (search != merger->getDict().at(type).end())
        return;

    if (type == Tools::RecType::INFO && add_hyperlinks)
    {
        val_text = Tools::addHyperlinks(
            merger->getDict().at(Tools::RecType::DIAL),
            val_text,
            true);
    }

    insertRecordToDict();
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeCHANGED()
{
    if (type == Tools::RecType::CELL ||
        type == Tools::RecType::DIAL ||
        type == Tools::RecType::BNAM ||
        type == Tools::RecType::SCTX)
        return;

    auto search = merger->getDict().at(type).find(key_text);
    if (search == merger->getDict().at(type).end())
        return;

    if (search->second == val_text)
        return;

    if (type == Tools::RecType::INFO && add_hyperlinks)
    {
        val_text = Tools::addHyperlinks(
            merger->getDict().at(Tools::RecType::DIAL),
            val_text,
            true);
    }

    insertRecordToDict();
}

//----------------------------------------------------------
void DictCreator::insertRecordToDict()
{
    if (dict.at(type).insert({ key_text, val_text }).second)
    {
        counter_created++;
    }
    else
    {
        auto search = dict.at(type).find(key_text);
        if (val_text != search->second)
        {
            // TODO
            auto new_unique =
                key_text + Tools::err[0] + "DOUBLED_" +
                std::to_string(counter_doubled) + Tools::err[1];

            dict[type].insert({ new_unique, val_text });
            counter_doubled++;
            counter_created++;
            Tools::addLog("Doubled " + Tools::getTypeName(type) + ": " + key_text + "\r\n");
        }
        else
        {
            counter_identical++;
        }
    }
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
