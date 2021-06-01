#include "dictcreator.hpp"

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & path
)
    : esm(path)
    , esm_ref(esm)
    , mode(Tools::CreatorMode::RAW)
    , merger(DictMerger())
{
    dict = Tools::initializeDict();

    if (esm.isLoaded())
        makeDict(true);
}

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & path,
    const std::string & path_ext
)
    : esm(path)
    , esm_ext(path_ext)
    , esm_ref(esm_ext)
    , mode(Tools::CreatorMode::BASE)
    , merger(DictMerger())
{
    dict = Tools::initializeDict();

    if (esm.isLoaded() &&
        esm_ext.isLoaded())
    {
        makeDict(isSameOrder());
    }
}

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & path,
    const DictMerger & merger,
    const Tools::CreatorMode mode
)
    : esm(path)
    , esm_ref(esm)
    , merger(merger)
    , mode(mode)
{
    dict = Tools::initializeDict();

    if (esm.isLoaded())
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
        makeDictCELL_Wilderness();
        makeDictCELL_Region();
        makeDictDIAL();
        makeDictBNAM();
        makeDictSCPT();
    }
    else
    {
        makeDictCELL_Unordered();
        makeDictCELL_Unordered_Wilderness();
        makeDictCELL_Unordered_Region();
        makeDictDIAL_Unordered();
        makeDictBNAM_Unordered();
        makeDictSCPT_Unordered();
    }

    makeDictGMST();
    makeDictFNAM();
    makeDictDESC();
    makeDictTEXT();
    makeDictRNAM();
    makeDictINDX();
    makeDictNPC_FLAG();
    makeDictINFO();

    if (!same_order)
        Tools::addLog("--> Check dictionary for \"MISSING\" keyword!\r\n"
                      "    Missing CELL and DIAL records needs to be added manually!\r\n");

    Tools::addLog("-----------------------------------------------\r\n");
}

//----------------------------------------------------------
bool DictCreator::isSameOrder()
{
    std::string ids;
    std::string ids_ext;

    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        ids += esm.getRecordId();
    }

    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        ids_ext += esm_ext.getRecordId();
    }

    return ids == ids_ext;
}

//----------------------------------------------------------
static bool ValidateSubRecord(
    const EsmReader::SubRecord & key,
    const EsmReader::SubRecord & value,
    const Tools::RecType type)
{
    switch (type)
    {
    case Tools::RecType::CELL:
        return
            key.exist &&
            key.text != "" &&
            value.exist &&
            value.text != "";
    }
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "CELL")
            continue;

        esm.setValue("NAME");
        esm_ref.selectRecord(i);
        esm_ref.setValue("NAME");
        if (esm.getValue().exist &&
            esm.getValue().text != "" &&
            esm_ref.getValue().exist &&
            esm_ref.getValue().text != "")
        {
            const auto & key_text = esm_ref.getValue().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::CELL;
            validateEntry({ key_text, val_text, type });
        }
    }
    printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Wilderness()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "GMST")
            continue;

        esm.setKey("NAME");
        esm.setValue("STRV");
        esm_ref.selectRecord(i);
        esm_ref.setKey("NAME");
        esm_ref.setValue("STRV");
        if (esm.getKey().text == "sDefaultCellname" &&
            esm.getValue().exist &&
            esm_ref.getKey().text == "sDefaultCellname" &&
            esm_ref.getValue().exist)
        {
            const auto & key_text = esm_ref.getValue().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::CELL;
            validateEntry({ key_text, val_text, type });
        }
    }
    printLogLine(Tools::RecType::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Unordered_Wilderness()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "GMST")
            continue;

        esm.setKey("NAME");
        esm.setValue("STRV");
        if (esm.getKey().text == "sDefaultCellname" &&
            esm.getValue().exist)
        {
            for (size_t k = 0; k < esm_ext.getRecords().size(); ++k)
            {
                esm_ext.selectRecord(k);
                if (esm_ext.getRecordId() != "GMST")
                    continue;

                esm_ext.setKey("NAME");
                esm_ext.setValue("STRV");
                if (esm_ext.getKey().text == "sDefaultCellname" &&
                    esm_ext.getValue().exist)
                {
                    const auto & key_text = esm_ext.getValue().text;
                    const auto & val_text = esm.getValue().text;
                    const auto & type = Tools::RecType::CELL;
                    validateEntry({ key_text, val_text, type });
                    break;
                }
            }
            break;
        }
    }
    printLogLine(Tools::RecType::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Region()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "REGN")
            continue;

        esm.setValue("FNAM");
        esm_ref.selectRecord(i);
        esm_ref.setValue("FNAM");
        if (esm.getValue().exist &&
            esm_ref.getValue().exist)
        {
            const auto & key_text = esm_ref.getValue().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::CELL;
            validateEntry({ key_text, val_text, type });
        }
    }
    printLogLine(Tools::RecType::Region);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Unordered_Region()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "REGN")
            continue;

        esm.setKey("NAME");
        esm.setValue("FNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            for (size_t k = 0; k < esm_ext.getRecords().size(); ++k)
            {
                esm_ext.selectRecord(k);
                if (esm_ext.getRecordId() != "REGN")
                    continue;

                esm_ext.setKey("NAME");
                esm_ext.setValue("FNAM");
                if (esm_ext.getKey().text == esm.getKey().text &&
                    esm_ext.getValue().exist)
                {
                    const auto & key_text = esm_ext.getValue().text;
                    const auto & val_text = esm.getValue().text;
                    const auto & type = Tools::RecType::CELL;
                    validateEntry({ key_text, val_text, type });
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
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "GMST")
            continue;

        esm.setKey("NAME");
        esm.setValue("STRV");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm.getKey().text.substr(0, 1) == "s")
        {
            const auto & key_text = esm.getKey().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::GMST;
            validateEntry({ key_text, val_text, type });
        }
    }
    printLogLine(Tools::RecType::GMST);
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "ACTI" ||
            esm.getRecordId() == "ALCH" ||
            esm.getRecordId() == "APPA" ||
            esm.getRecordId() == "ARMO" ||
            esm.getRecordId() == "BOOK" ||
            esm.getRecordId() == "BSGN" ||
            esm.getRecordId() == "CLAS" ||
            esm.getRecordId() == "CLOT" ||
            esm.getRecordId() == "CONT" ||
            esm.getRecordId() == "CREA" ||
            esm.getRecordId() == "DOOR" ||
            esm.getRecordId() == "FACT" ||
            esm.getRecordId() == "INGR" ||
            esm.getRecordId() == "LIGH" ||
            esm.getRecordId() == "LOCK" ||
            esm.getRecordId() == "MISC" ||
            esm.getRecordId() == "NPC_" ||
            esm.getRecordId() == "PROB" ||
            esm.getRecordId() == "RACE" ||
            esm.getRecordId() == "REGN" ||
            esm.getRecordId() == "REPA" ||
            esm.getRecordId() == "SKIL" ||
            esm.getRecordId() == "SPEL" ||
            esm.getRecordId() == "WEAP")
        {
            esm.setKey("NAME");
            esm.setValue("FNAM");
            if (esm.getKey().exist &&
                esm.getValue().exist &&
                esm.getKey().text != "player")
            {
                const auto & key_text = esm.getRecordId() + Tools::sep[0] + esm.getKey().text;
                const auto & val_text = esm.getValue().text;
                const auto & type = Tools::RecType::FNAM;
                validateEntry({ key_text, val_text, type });
            }
        }
    }
    printLogLine(Tools::RecType::FNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "BSGN" ||
            esm.getRecordId() == "CLAS" ||
            esm.getRecordId() == "RACE")
        {
            esm.setKey("NAME");
            esm.setValue("DESC");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                const auto & key_text = esm.getRecordId() + Tools::sep[0] + esm.getKey().text;
                const auto & val_text = esm.getValue().text;
                const auto & type = Tools::RecType::DESC;
                validateEntry({ key_text, val_text, type });
            }
        }
    }
    printLogLine(Tools::RecType::DESC);
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "BOOK")
            continue;

        esm.setKey("NAME");
        esm.setValue("TEXT");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            const auto & key_text = esm.getKey().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::TEXT;
            validateEntry({ key_text, val_text, type });
        }
    }
    printLogLine(Tools::RecType::TEXT);
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "FACT")
            continue;

        esm.setKey("NAME");
        esm.setValue("RNAM");
        if (!esm.getKey().exist)
            continue;

        while (esm.getValue().exist)
        {
            const auto & key_text = esm.getKey().text + Tools::sep[0] + std::to_string(esm.getValue().counter);
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::RNAM;
            validateEntry({ key_text, val_text, type });
            esm.setNextValue("RNAM");
        }
    }
    printLogLine(Tools::RecType::RNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "SKIL" ||
            esm.getRecordId() == "MGEF")
        {
            esm.setKey("INDX");
            esm.setValue("DESC");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                const auto & key_text = esm.getRecordId() + Tools::sep[0] + Tools::getINDX(esm.getKey().content);
                const auto & val_text = esm.getValue().text;
                const auto & type = Tools::RecType::INDX;
                validateEntry({ key_text, val_text, type });
            }
        }
    }
    printLogLine(Tools::RecType::INDX);
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "DIAL")
            continue;

        esm.setKey("DATA");
        esm.setValue("NAME");
        esm_ref.selectRecord(i);
        esm_ref.setKey("DATA");
        esm_ref.setValue("NAME");
        if (Tools::getDialogType(esm.getKey().content) == "T" &&
            esm.getValue().exist &&
            Tools::getDialogType(esm_ref.getKey().content) == "T" &&
            esm_ref.getValue().exist)
        {
            const auto & key_text = esm_ref.getValue().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::DIAL;
            validateEntry({ key_text, val_text, type });
        }
    }
    printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
void DictCreator::makeDictNPC_FLAG()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "NPC_")
            continue;

        esm.setKey("NAME");
        esm.setValue("FLAG");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            const auto & key_text = esm.getKey().text;
            const auto & val_text =
                ((Tools::convertStringByteArrayToUInt(esm.getValue().content) & 0x0001) != 0)
                ? "F" : "M";
            const auto & type = Tools::RecType::NPC_FLAG;
            validateEntry({ key_text, val_text, type });
        }
    }
    printLogLine(Tools::RecType::NPC_FLAG);
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
    std::string key_prefix;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "DIAL")
        {
            esm.setKey("DATA");
            esm.setValue("NAME");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                key_prefix = Tools::getDialogType(esm.getKey().content) + Tools::sep[0] +
                    translateDialogTopic(esm.getValue().text);
            }
        }

        if (esm.getRecordId() == "INFO")
        {
            esm.setKey("INAM");
            if (!esm.getKey().exist)
                continue;

            esm.setValue("ONAM");
            const auto npc_text = esm.getValue().text;

            esm.setValue("NAME");
            if (!esm.getValue().exist)
                continue;

            const auto & key_text = key_prefix + Tools::sep[0] + esm.getKey().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::INFO;
            validateEntry({ key_text, val_text, type, npc_text });
        }
    }
    printLogLine(Tools::RecType::INFO);
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "INFO")
            continue;

        esm.setKey("INAM");
        esm.setValue("BNAM");
        esm_ref.selectRecord(i);
        esm_ref.setKey("INAM");
        esm_ref.setValue("BNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm_ref.getKey().exist &&
            esm_ref.getValue().exist)
        {
            const auto & message = makeScriptMessages(esm.getValue().text);
            const auto & message_ext = makeScriptMessages(esm_ref.getValue().text);
            if (message.size() != message_ext.size())
                continue;

            for (size_t k = 0; k < message.size(); ++k)
            {
                const auto & key_text = esm_ref.getKey().text + Tools::sep[0] + message_ext.at(k);
                const auto & val_text = esm.getKey().text + Tools::sep[0] + message.at(k);
                const auto & type = Tools::RecType::BNAM;
                validateEntry({ key_text, val_text, type });
            }
        }
    }
    printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "SCPT")
            continue;

        esm.setKey("SCHD");
        esm.setValue("SCTX");
        esm_ref.selectRecord(i);
        esm_ref.setKey("SCHD");
        esm_ref.setValue("SCTX");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm_ref.getKey().exist &&
            esm_ref.getValue().exist)
        {
            const auto & message = makeScriptMessages(esm.getValue().text);
            const auto & message_ext = makeScriptMessages(esm_ref.getValue().text);
            if (message.size() != message_ext.size())
                continue;

            for (size_t k = 0; k < message.size(); ++k)
            {
                const auto & key_text = esm_ref.getKey().text + Tools::sep[0] + message_ext.at(k);
                const auto & val_text = esm.getKey().text + Tools::sep[0] + message.at(k);
                const auto & type = Tools::RecType::SCTX;
                validateEntry({ key_text, val_text, type });
            }
        }
    }
    printLogLine(Tools::RecType::SCTX);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Unordered()
{
    auto patterns_ext = makeDictCELL_Unordered_PatternsExt();
    const auto & patterns = makeDictCELL_Unordered_Patterns();

    resetCounters();
    for (size_t i = 0; i < patterns_ext.size(); ++i)
    {
        auto search = patterns.find(patterns_ext[i].str);
        if (search != patterns.end())
        {
            esm.selectRecord(search->second);
            esm.setValue("NAME");
            esm_ext.selectRecord(patterns_ext[i].pos);
            esm_ext.setValue("NAME");
            if (esm.getValue().exist &&
                esm.getValue().text != "" &&
                esm_ext.getValue().exist &&
                esm_ext.getValue().text != "")
            {
                const auto & key_text = esm_ext.getValue().text;
                const auto & val_text = esm.getValue().text;
                const auto & type = Tools::RecType::CELL;
                validateEntry({ key_text, val_text, type });
            }
        }
        else
        {
            patterns_ext[i].missing = true;
            counter_missing++;
        }
    }
    makeDictCELL_Unordered_AddMissing(patterns_ext);
    printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
DictCreator::PatternsExt DictCreator::makeDictCELL_Unordered_PatternsExt()
{
    PatternsExt patterns_ext;
    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        if (esm_ext.getRecordId() != "CELL")
            continue;

        esm_ext.setValue("NAME");
        if (esm_ext.getValue().exist &&
            esm_ext.getValue().text != "")
        {
            patterns_ext.push_back({ makeDictCELL_Unordered_Pattern(esm_ext), i, false });
        }
    }
    return patterns_ext;
}

//----------------------------------------------------------
DictCreator::Patterns DictCreator::makeDictCELL_Unordered_Patterns()
{
    Patterns patterns;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "CELL")
            continue;

        esm.setValue("NAME");
        if (esm.getValue().exist &&
            esm.getValue().text != "")
        {
            patterns.insert({ makeDictCELL_Unordered_Pattern(esm), i });
        }
    }
    return patterns;
}

//----------------------------------------------------------
std::string DictCreator::makeDictCELL_Unordered_Pattern(EsmReader & esm_cur)
{
    /* pattern is the DATA and combined ids of all objects in a cell */

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
void DictCreator::makeDictCELL_Unordered_AddMissing(const PatternsExt & patterns_ext)
{
    for (size_t i = 0; i < patterns_ext.size(); ++i)
    {
        if (!patterns_ext[i].missing)
            continue;

        esm_ext.selectRecord(patterns_ext[i].pos);
        esm_ext.setValue("NAME");
        if (esm_ext.getValue().exist &&
            esm_ext.getValue().text != "")
        {
            const auto & key_text = esm_ext.getValue().text;
            const auto & val_text = Tools::err[0] + "MISSING" + Tools::err[1];
            const auto & type = Tools::RecType::CELL;
            validateEntry({ key_text, val_text, type });
            Tools::addLog("Missing CELL: " + esm_ext.getValue().text + "\r\n");
        }
    }
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL_Unordered()
{
    auto patterns_ext = makeDictDIAL_Unordered_PatternsExt();
    const auto & patterns = makeDictDIAL_Unordered_Patterns();

    resetCounters();
    for (size_t i = 0; i < patterns_ext.size(); ++i)
    {
        auto search = patterns.find(patterns_ext[i].str);
        if (search != patterns.end())
        {
            esm.selectRecord(search->second);
            esm.setValue("NAME");
            esm_ext.selectRecord(patterns_ext[i].pos);
            esm_ext.setValue("NAME");
            if (esm.getValue().exist &&
                esm_ext.getValue().exist)
            {
                const auto & key_text = esm_ext.getValue().text;
                const auto & val_text = esm.getValue().text;
                const auto & type = Tools::RecType::DIAL;
                validateEntry({ key_text, val_text, type });
            }
        }
        else
        {
            patterns_ext[i].missing = true;
            counter_missing++;
        }
    }
    makeDictDIAL_Unordered_AddMissing(patterns_ext);
    printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
DictCreator::PatternsExt DictCreator::makeDictDIAL_Unordered_PatternsExt()
{
    PatternsExt patterns_ext;
    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        if (esm_ext.getRecordId() != "DIAL")
            continue;

        esm_ext.setKey("DATA");
        if (Tools::getDialogType(esm_ext.getKey().content) == "T")
        {
            patterns_ext.push_back({ makeDictDIAL_Unordered_Pattern(esm_ext, i), i, false });
        }
    }
    return patterns_ext;
}

//----------------------------------------------------------
DictCreator::Patterns DictCreator::makeDictDIAL_Unordered_Patterns()
{
    Patterns patterns;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "DIAL")
            continue;

        esm.setKey("DATA");
        if (Tools::getDialogType(esm.getKey().content) == "T")
        {
            patterns.insert({ makeDictDIAL_Unordered_Pattern(esm, i), i });
        }
    }
    return patterns;
}

//----------------------------------------------------------
std::string DictCreator::makeDictDIAL_Unordered_Pattern(EsmReader & esm_cur, size_t i)
{
    /* pattern is the INAM and SCVR from next INFO record */

    std::string pattern;
    esm_cur.selectRecord(i + 1);
    esm_cur.setValue("INAM");
    pattern += esm_cur.getValue().content;
    esm_cur.setValue("SCVR");
    pattern += esm_cur.getValue().content;
    return pattern;
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL_Unordered_AddMissing(const PatternsExt & patterns_ext)
{
    for (size_t i = 0; i < patterns_ext.size(); ++i)
    {
        if (!patterns_ext[i].missing)
            continue;

        esm_ext.selectRecord(patterns_ext[i].pos);
        esm_ext.setValue("NAME");
        if (!esm_ext.getValue().exist)
            continue;

        const auto & key_text = esm_ext.getValue().text;
        const auto & val_text = Tools::err[0] + "MISSING" + Tools::err[1];
        const auto & type = Tools::RecType::DIAL;
        validateEntry({ key_text, val_text, type });
        Tools::addLog("Missing DIAL: " + esm_ext.getValue().text + "\r\n");
    }
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM_Unordered()
{
    auto patterns_ext = makeDictBNAM_Unordered_PatternsExt();
    const auto & patterns = makeDictBNAM_Unordered_Patterns();

    resetCounters();
    for (size_t i = 0; i < patterns_ext.size(); ++i)
    {
        auto search = patterns.find(patterns_ext[i].str);
        if (search == patterns.end())
            continue;

        esm.selectRecord(search->second);
        esm.setKey("INAM");
        esm.setValue("BNAM");
        esm_ext.selectRecord(patterns_ext[i].pos);
        esm_ext.setKey("INAM");
        esm_ext.setValue("BNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm_ext.getKey().exist &&
            esm_ext.getValue().exist)
        {
            const auto & message = makeScriptMessages(esm.getValue().text);
            const auto & message_ext = makeScriptMessages(esm_ext.getValue().text);
            if (message.size() != message_ext.size())
                continue;

            for (size_t k = 0; k < message.size(); ++k)
            {
                const auto & key_text = esm_ext.getKey().text + Tools::sep[0] + message_ext.at(k);
                const auto & val_text = esm_ext.getKey().text + Tools::sep[0] + message.at(k);
                const auto & type = Tools::RecType::BNAM;
                validateEntry({ key_text, val_text, type });
            }
        }
    }
    printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
DictCreator::PatternsExt DictCreator::makeDictBNAM_Unordered_PatternsExt()
{
    PatternsExt patterns_ext;
    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        if (esm_ext.getRecordId() != "INFO")
            continue;

        esm_ext.setKey("INAM");
        if (!esm_ext.getKey().exist)
            continue;

        patterns_ext.push_back({ esm_ext.getKey().text, i, false });
    }
    return patterns_ext;
}

//----------------------------------------------------------
DictCreator::Patterns DictCreator::makeDictBNAM_Unordered_Patterns()
{
    Patterns patterns;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "INFO")
            continue;

        esm.setKey("INAM");
        if (!esm.getKey().exist)
            continue;

        patterns.insert({ esm.getKey().text, i });
    }
    return patterns;
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT_Unordered()
{
    auto patterns_ext = makeDictSCPT_Unordered_PatternsExt();
    const auto & patterns = makeDictSCPT_Unordered_Patterns();

    resetCounters();
    for (size_t i = 0; i < patterns_ext.size(); ++i)
    {
        auto search = patterns.find(patterns_ext[i].str);
        if (search == patterns.end())
            continue;

        esm.selectRecord(search->second);
        esm.setKey("SCHD");
        esm.setValue("SCTX");
        esm_ext.selectRecord(patterns_ext[i].pos);
        esm_ext.setKey("SCHD");
        esm_ext.setValue("SCTX");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm_ext.getKey().exist &&
            esm_ext.getValue().exist)
        {
            const auto & message = makeScriptMessages(esm.getValue().text);
            const auto & message_ext = makeScriptMessages(esm_ext.getValue().text);
            if (message.size() != message_ext.size())
                continue;

            for (size_t k = 0; k < message.size(); ++k)
            {
                const auto & key_text = esm_ext.getKey().text + Tools::sep[0] + message_ext.at(k);
                const auto & val_text = esm.getKey().text + Tools::sep[0] + message.at(k);
                const auto & type = Tools::RecType::SCTX;
                validateEntry({ key_text, val_text, type });
            }
        }
    }
    printLogLine(Tools::RecType::SCTX);
}

//----------------------------------------------------------
DictCreator::PatternsExt DictCreator::makeDictSCPT_Unordered_PatternsExt()
{
    PatternsExt patterns_ext;
    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        if (esm_ext.getRecordId() != "SCPT")
            continue;

        esm_ext.setKey("SCHD");
        if (!esm_ext.getKey().exist)
            continue;

        patterns_ext.push_back({ esm_ext.getKey().text, i , false });
    }
    return patterns_ext;
}

//----------------------------------------------------------
DictCreator::Patterns DictCreator::makeDictSCPT_Unordered_Patterns()
{
    Patterns patterns;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "SCPT")
            continue;

        esm.setKey("SCHD");
        if (!esm.getKey().exist)
            continue;

        patterns.insert({ esm.getKey().text, i });
    }
    return patterns;
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
void DictCreator::validateEntry(const Entry & entry)
{
    counter_all++;

    if (mode == Tools::CreatorMode::RAW ||
        mode == Tools::CreatorMode::BASE)
    {
        insertRecordToDict(entry);
    }

    if (mode == Tools::CreatorMode::ALL)
    {
        validateRecordForModeALL(entry);
    }

    if (mode == Tools::CreatorMode::NOTFOUND)
    {
        validateRecordForModeNOT(entry);
    }

    if (mode == Tools::CreatorMode::CHANGED)
    {
        validateRecordForModeCHANGED(entry);
    }
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeALL(const Entry & entry)
{
    std::string val_text = entry.val_text;
    if (entry.type == Tools::RecType::CELL ||
        entry.type == Tools::RecType::DIAL ||
        entry.type == Tools::RecType::BNAM ||
        entry.type == Tools::RecType::SCTX)
    {
        auto search = merger.getDict().at(entry.type).find(entry.key_text);
        if (search != merger.getDict().at(entry.type).end())
        {
            val_text = search->second;
        }
    }

    insertRecordToDict({ entry.key_text, val_text, entry.type });
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeNOT(const Entry & entry)
{
    auto search = merger.getDict().at(entry.type).find(entry.key_text);
    if (search != merger.getDict().at(entry.type).end())
        return;

    makeAnnotations(entry);
    insertRecordToDict(entry);
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeCHANGED(const Entry & entry)
{
    if (entry.type == Tools::RecType::CELL ||
        entry.type == Tools::RecType::DIAL ||
        entry.type == Tools::RecType::BNAM ||
        entry.type == Tools::RecType::SCTX)
        return;

    auto search = merger.getDict().at(entry.type).find(entry.key_text);
    if (search == merger.getDict().at(entry.type).end())
        return;

    if (search->second == entry.val_text)
        return;

    makeAnnotations(entry);
    insertRecordToDict(entry);
}

//----------------------------------------------------------
void DictCreator::makeAnnotations(const Entry & entry)
{
    if (entry.type != Tools::RecType::INFO)
        return;

    std::string annotations;

    annotations += "\r\nHyperlinks 1:";
    annotations += Tools::addHyperlinks(
        merger.getDict().at(Tools::RecType::DIAL),
        entry.val_text,
        true);

    annotations += "\r\nHyperlinks 2:";
    annotations += Tools::addHyperlinks(
        dict.at(Tools::RecType::DIAL),
        entry.val_text,
        false);

    annotations += "\r\nGlossary:";
    annotations += Tools::addHyperlinks(
        merger.getDict().at(Tools::RecType::Glossary),
        entry.val_text,
        true);

    std::string npc_flag;
    auto search_merger = merger.getDict().at(Tools::RecType::NPC_FLAG).find(entry.npc_text);
    if (search_merger != merger.getDict().at(Tools::RecType::NPC_FLAG).end())
    {
        npc_flag = search_merger->second;
    }

    auto search_dict = dict.at(Tools::RecType::NPC_FLAG).find(entry.npc_text);
    if (search_dict != dict.at(Tools::RecType::NPC_FLAG).end())
    {
        npc_flag = search_dict->second;
    }

    annotations += "\r\nSpeaker: ";
    annotations += npc_flag;

    dict.at(Tools::RecType::Annotations).insert({ entry.key_text, annotations });
}

//----------------------------------------------------------
void DictCreator::insertRecordToDict(const Entry & entry)
{
    if (dict.at(entry.type).insert({ entry.key_text, entry.val_text }).second)
    {
        counter_created++;
    }
    else
    {
        auto search = dict.at(entry.type).find(entry.key_text);
        if (entry.val_text != search->second)
        {
            std::string key_text =
                entry.key_text + Tools::err[0] + "DOUBLED_" +
                std::to_string(counter_doubled) + Tools::err[1];

            dict.at(entry.type).insert({ key_text, entry.val_text });
            counter_doubled++;
            counter_created++;
            Tools::addLog("Warning: Doubled " + Tools::getTypeName(entry.type) + ": " + entry.key_text + "\r\n");
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
    std::string type_name = Tools::getTypeName(type);
    type_name.resize(12, ' ');

    std::ostringstream ss;
    ss << type_name << std::setw(5) << std::to_string(counter_created) << " / ";

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
std::string DictCreator::translateDialogTopic(std::string to_translate)
{
    if (mode == Tools::CreatorMode::ALL ||
        mode == Tools::CreatorMode::NOTFOUND ||
        mode == Tools::CreatorMode::CHANGED)
    {
        auto search = merger.getDict().at(Tools::RecType::DIAL).find(to_translate);
        if (search != merger.getDict().at(Tools::RecType::DIAL).end())
        {
            return search->second;
        }
    }
    return to_translate;
}

//----------------------------------------------------------
std::vector<std::string> DictCreator::makeScriptMessages(const std::string & script_text)
{
    std::vector<std::string> messages;
    std::string line;
    std::string line_lc;
    std::istringstream ss(script_text);

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
