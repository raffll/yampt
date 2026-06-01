#include "dictcreator.hpp"

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & plugin_path,
    const Tools::Dict * base_dict
)
    : esm(plugin_path)
    , esm_ref(esm)
    , base_dict(base_dict)
    , is_make_mode(true)
{
    dict = Tools::initializeDict();

    if (esm.isLoaded())
        makeDictForMake();
}

//----------------------------------------------------------
DictCreator::DictCreator(
    const std::string & path,
    const std::string & path_ext
)
    : esm(path)
    , esm_ext(path_ext)
    , esm_ref(esm_ext)
    , is_make_mode(false)
{
    dict = Tools::initializeDict();

    if (esm.isLoaded() &&
        esm_ext.isLoaded())
    {
        makeDictForBase(isSameOrder());
    }
}

//----------------------------------------------------------
void DictCreator::makeDictForMake()
{
    Tools::addLog("-----------------------------------------------\r\n"
                  "          Created / Missing / Identical /   All\r\n"
                  "-----------------------------------------------\r\n");

    makeDictCELL();
    makeDictCELL_Default();
    makeDictCELL_REGN();
    makeDictDIAL();
    makeDictGMST();
    makeDictFNAM();
    makeDictDESC();
    makeDictTEXT();
    makeDictRNAM();
    makeDictINDX();
    makeDictNPC_FLAG();
    makeDictINFO();
    makeDictScript({ "INFO", "INAM", "BNAM", Tools::RecType::BNAM });
    makeDictScript({ "SCPT", "SCHD", "SCTX", Tools::RecType::SCTX });

    Tools::addLog("-----------------------------------------------\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictForBase(const bool same_order)
{
    Tools::addLog("-----------------------------------------------\r\n"
                  "          Created / Missing / Identical /   All\r\n"
                  "-----------------------------------------------\r\n");

    if (same_order)
    {
        makeDictCELL();
        makeDictCELL_Default();
        makeDictCELL_REGN();
        makeDictDIAL();
        makeDictScript({ "INFO", "INAM", "BNAM", Tools::RecType::BNAM });
        makeDictScript({ "SCPT", "SCHD", "SCTX", Tools::RecType::SCTX });
    }
    else
    {
        makeDictCELL_Unordered();
        makeDictCELL_Unordered_Default();
        makeDictCELL_Unordered_REGN();
        makeDictDIAL_Unordered();
        makeDictScript_Unordered({ "INFO", "INAM", "BNAM", Tools::RecType::BNAM });
        makeDictScript_Unordered({ "SCPT", "SCHD", "SCTX", Tools::RecType::SCTX });
    }

    makeDictGMST();
    makeDictFNAM();
    makeDictDESC();
    makeDictTEXT();
    makeDictRNAM();
    makeDictINDX();
    makeDictNPC_FLAG();
    makeDictINFO();
    makeDictFNAM_Glossary();

    if (!same_order)
        Tools::addLog("--> Check dictionary for \"MISSING\" keyword!\r\n"
                      "    Missing CELL and DIAL records needs to be added manually!\r\n");

    Tools::addLog("-----------------------------------------------\r\n");
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Unordered_Default()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecord().id != "GMST")
            continue;

        esm.setKey("NAME");
        esm.setValue("STRV");
        if (esm.getKey().text == "sDefaultCellname" &&
            esm.getValue().exist)
        {
            for (size_t k = 0; k < esm_ext.getRecords().size(); ++k)
            {
                esm_ext.selectRecord(k);
                if (esm_ext.getRecord().id != "GMST")
                    continue;

                esm_ext.setKey("NAME");
                esm_ext.setValue("STRV");
                if (esm_ext.getKey().text == "sDefaultCellname" &&
                    esm_ext.getValue().exist)
                {
                    const auto & key_text = esm_ext.getValue().text;
                    const auto & val_text = esm.getValue().text;
                    insertRecordToDict(key_text, val_text, Tools::RecType::CELL);
                    break;
                }
            }
            break;
        }
    }
    printLogLine(Tools::RecType::Default);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Unordered_REGN()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecord().id != "REGN")
            continue;

        esm.setKey("NAME");
        esm.setValue("FNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            for (size_t k = 0; k < esm_ext.getRecords().size(); ++k)
            {
                esm_ext.selectRecord(k);
                if (esm_ext.getRecord().id != "REGN")
                    continue;

                esm_ext.setKey("NAME");
                esm_ext.setValue("FNAM");
                if (esm_ext.getKey().text == esm.getKey().text &&
                    esm_ext.getValue().exist)
                {
                    const auto & key_text = esm_ext.getValue().text;
                    const auto & val_text = esm.getValue().text;
                    insertRecordToDict(key_text, val_text, Tools::RecType::CELL);
                    break;
                }
            }
        }
    }
    printLogLine(Tools::RecType::REGN);
}

//----------------------------------------------------------
bool DictCreator::isSameOrder()
{
    if (esm.getRecords().size() != esm_ext.getRecords().size())
        return false;

    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        if (esm.getRecords()[i].id != esm_ext.getRecords()[i].id)
            return false;
    }
    return true;
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
    resetCounters();

    if (is_make_mode)
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "CELL")
                continue;

            esm.setValue("NAME");
            if (esm.getValue().exist &&
                esm.getValue().text != "")
            {
                const auto & id = esm.getValue().text;
                const auto & original = esm.getValue().text;
                insertRecord(id, original, Tools::RecType::CELL);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "CELL")
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
                insertRecordToDict(key_text, val_text, Tools::RecType::CELL);
            }
        }
    }

    printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_Default()
{
    resetCounters();

    if (is_make_mode)
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "GMST")
                continue;

            esm.setKey("NAME");
            esm.setValue("STRV");
            if (esm.getKey().text == "sDefaultCellname" &&
                esm.getValue().exist)
            {
                const auto & id = esm.getValue().text;
                const auto & original = esm.getValue().text;
                insertRecord(id, original, Tools::RecType::CELL);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "GMST")
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
                insertRecordToDict(key_text, val_text, Tools::RecType::CELL);
            }
        }
    }

    printLogLine(Tools::RecType::Default);
}

//----------------------------------------------------------
void DictCreator::makeDictCELL_REGN()
{
    resetCounters();

    if (is_make_mode)
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "REGN")
                continue;

            esm.setValue("FNAM");
            if (esm.getValue().exist)
            {
                const auto & id = esm.getValue().text;
                const auto & original = esm.getValue().text;
                insertRecord(id, original, Tools::RecType::CELL);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "REGN")
                continue;

            esm.setValue("FNAM");
            esm_ref.selectRecord(i);
            esm_ref.setValue("FNAM");
            if (esm.getValue().exist &&
                esm_ref.getValue().exist)
            {
                const auto & key_text = esm_ref.getValue().text;
                const auto & val_text = esm.getValue().text;
                insertRecordToDict(key_text, val_text, Tools::RecType::CELL);
            }
        }
    }

    printLogLine(Tools::RecType::REGN);
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecord().id != "GMST")
            continue;

        esm.setKey("NAME");
        esm.setValue("STRV");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm.getKey().text.substr(0, 1) == "s")
        {
            const auto & id = esm.getKey().text;
            const auto & text = esm.getValue().text;

            if (is_make_mode)
                insertRecord(id, text, Tools::RecType::GMST);
            else
                insertRecordToDict(id, text, Tools::RecType::GMST);
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
        if (!Tools::isFNAM(esm.getRecord().id))
            continue;

        esm.setKey("NAME");
        esm.setValue("FNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm.getKey().text != "player")
        {
            const auto & id = esm.getRecord().id + "^" + esm.getKey().text;
            const auto & text = esm.getValue().text;

            if (is_make_mode)
                insertRecord(id, text, Tools::RecType::FNAM);
            else
                insertRecordToDict(id, text, Tools::RecType::FNAM);
        }
    }
    printLogLine(Tools::RecType::FNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM_Glossary()
{
    std::unordered_map<std::string, size_t> ext_index;
    for (size_t k = 0; k < esm_ext.getRecords().size(); ++k)
    {
        esm_ext.selectRecord(k);
        if (!Tools::isFNAM(esm_ext.getRecord().id))
            continue;

        esm_ext.setKey("NAME");
        if (!esm_ext.getKey().exist)
            continue;

        ext_index.insert({ esm_ext.getRecord().id + "^" + esm_ext.getKey().text, k });
    }

    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (!Tools::isFNAM(esm.getRecord().id))
            continue;

        esm.setKey("NAME");
        esm.setValue("FNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm.getValue().text != "")
        {
            auto search = ext_index.find(esm.getRecord().id + "^" + esm.getKey().text);
            if (search == ext_index.end())
                continue;

            esm_ext.selectRecord(search->second);
            esm_ext.setValue("FNAM");
            if (esm_ext.getValue().exist &&
                esm_ext.getValue().text != "")
            {
                const auto & key_text = esm_ext.getValue().text;
                const auto & val_text =
                    esm.getValue().text + " "
                    + esm.getRecord().id + "^"
                    + esm.getKey().text;
                insertRecordToDict(key_text, val_text, Tools::RecType::Glossary);
            }
        }
    }
    printLogLine(Tools::RecType::Glossary);
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecord().id == "BSGN" ||
            esm.getRecord().id == "CLAS" ||
            esm.getRecord().id == "RACE")
        {
            esm.setKey("NAME");
            esm.setValue("DESC");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                const auto & id = esm.getRecord().id + "^" + esm.getKey().text;
                const auto & text = esm.getValue().text;

                if (is_make_mode)
                    insertRecord(id, text, Tools::RecType::DESC);
                else
                    insertRecordToDict(id, text, Tools::RecType::DESC);
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
        if (esm.getRecord().id != "BOOK")
            continue;

        esm.setKey("NAME");
        esm.setValue("TEXT");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            const auto & id = esm.getKey().text;
            const auto & text = esm.getValue().text;

            if (is_make_mode)
                insertRecord(id, text, Tools::RecType::TEXT);
            else
                insertRecordToDict(id, text, Tools::RecType::TEXT);
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
        if (esm.getRecord().id != "FACT")
            continue;

        esm.setKey("NAME");
        esm.setValue("RNAM");
        if (!esm.getKey().exist)
            continue;

        while (esm.getValue().exist)
        {
            const auto & id = esm.getKey().text + "^" + std::to_string(esm.getValue().counter);
            const auto & text = esm.getValue().text;

            if (is_make_mode)
                insertRecord(id, text, Tools::RecType::RNAM);
            else
                insertRecordToDict(id, text, Tools::RecType::RNAM);

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
        if (esm.getRecord().id == "SKIL" ||
            esm.getRecord().id == "MGEF")
        {
            esm.setKey("INDX");
            esm.setValue("DESC");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                const auto & id = esm.getRecord().id + "^" + Tools::getINDX(esm.getKey().content);
                const auto & text = esm.getValue().text;

                if (is_make_mode)
                    insertRecord(id, text, Tools::RecType::INDX);
                else
                    insertRecordToDict(id, text, Tools::RecType::INDX);
            }
        }
    }
    printLogLine(Tools::RecType::INDX);
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
    resetCounters();

    if (is_make_mode)
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "DIAL")
                continue;

            esm.setKey("DATA");
            esm.setValue("NAME");
            if (Tools::getDialogType(esm.getKey().content) == "T" &&
                esm.getValue().exist)
            {
                const auto & id = esm.getValue().text;
                const auto & original = esm.getValue().text;
                insertRecord(id, original, Tools::RecType::DIAL);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != "DIAL")
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
                insertRecordToDict(key_text, val_text, Tools::RecType::DIAL);
            }
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
        if (esm.getRecord().id != "NPC_")
            continue;

        esm.setKey("NAME");
        esm.setValue("FLAG");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            const auto & id = esm.getKey().text;
            const auto & text =
                ((Tools::convertStringByteArrayToUInt(esm.getValue().content) & 0x0001) != 0)
                ? "F" : "M";

            if (is_make_mode)
                insertRecord(id, text, Tools::RecType::NPC_FLAG);
            else
                insertRecordToDict(id, text, Tools::RecType::NPC_FLAG);
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
        if (esm.getRecord().id == "DIAL")
        {
            esm.setKey("DATA");
            esm.setValue("NAME");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                key_prefix = Tools::getDialogType(esm.getKey().content) + "^" +
                    translateDialogTopic(esm.getValue().text);
            }
        }

        if (esm.getRecord().id == "INFO")
        {
            esm.setKey("INAM");
            if (!esm.getKey().exist)
                continue;

            esm.setValue("NAME");
            if (!esm.getValue().exist)
                continue;

            const auto & id = key_prefix + "^" + esm.getKey().text;
            const auto & text = esm.getValue().text;

            if (is_make_mode)
                insertRecord(id, text, Tools::RecType::INFO);
            else
                insertRecordToDict(id, text, Tools::RecType::INFO);
        }
    }
    printLogLine(Tools::RecType::INFO);
}

//----------------------------------------------------------
void DictCreator::makeDictScript(const IDs & ids)
{
    resetCounters();

    if (is_make_mode)
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != ids.rec_id)
                continue;

            esm.setKey(ids.key_id);
            esm.setValue(ids.val_id);
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                const auto & messages = makeScriptMessages(esm.getValue().text);
                for (size_t k = 0; k < messages.size(); ++k)
                {
                    const auto & id = esm.getKey().text + "^" + messages.at(k);
                    const auto & original = esm.getKey().text + "^" + messages.at(k);
                    insertRecord(id, original, ids.type);
                }
            }
        }
    }
    else
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecord().id != ids.rec_id)
                continue;

            esm.setKey(ids.key_id);
            esm.setValue(ids.val_id);
            esm_ref.selectRecord(i);
            esm_ref.setKey(ids.key_id);
            esm_ref.setValue(ids.val_id);
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
                    const auto & key_text = esm_ref.getKey().text + "^" + message_ext.at(k);
                    const auto & val_text = esm.getKey().text + "^" + message.at(k);
                    insertRecordToDict(key_text, val_text, ids.type);
                }
            }
        }
    }

    printLogLine(ids.type);
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
                insertRecordToDict(key_text, val_text, Tools::RecType::CELL);
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
                insertRecordToDict(key_text, val_text, Tools::RecType::DIAL);
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
DictCreator::PatternsExt DictCreator::makeDictCELL_Unordered_PatternsExt()
{
    PatternsExt patterns_ext;
    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        if (esm_ext.getRecord().id != "CELL")
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
DictCreator::PatternsExt DictCreator::makeDictDIAL_Unordered_PatternsExt()
{
    PatternsExt patterns_ext;
    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        if (esm_ext.getRecord().id != "DIAL")
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
DictCreator::Patterns DictCreator::makeDictCELL_Unordered_Patterns()
{
    Patterns patterns;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecord().id != "CELL")
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
DictCreator::Patterns DictCreator::makeDictDIAL_Unordered_Patterns()
{
    Patterns patterns;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecord().id != "DIAL")
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
std::string DictCreator::makeDictCELL_Unordered_Pattern(EsmReader & esm_cur)
{
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
std::string DictCreator::makeDictDIAL_Unordered_Pattern(EsmReader & esm_cur, size_t i)
{
    std::string pattern;
    esm_cur.selectRecord(i + 1);
    esm_cur.setValue("INAM");
    pattern += esm_cur.getValue().content;
    esm_cur.setValue("SCVR");
    pattern += esm_cur.getValue().content;
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
        if (!esm_ext.getValue().exist)
            continue;

        if (esm_ext.getValue().text == "")
            continue;

        const auto & key_text = esm_ext.getValue().text;
        const auto & val_text = "MISSING";
        insertRecordToDict(key_text, val_text, Tools::RecType::CELL);
        Tools::addLog("Missing CELL: " + esm_ext.getValue().text + "\r\n");
    }
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
        const auto & val_text = "MISSING";
        insertRecordToDict(key_text, val_text, Tools::RecType::DIAL);
        Tools::addLog("Missing DIAL: " + esm_ext.getValue().text + "\r\n");
    }
}

//----------------------------------------------------------
void DictCreator::makeDictScript_Unordered(const IDs & ids)
{
    auto patterns_ext = makeDict_Unordered_PatternsExt(ids);
    const auto & patterns = makeDict_Unordered_Patterns(ids);

    resetCounters();
    for (size_t i = 0; i < patterns_ext.size(); ++i)
    {
        auto search = patterns.find(patterns_ext[i].str);
        if (search == patterns.end())
            continue;

        esm.selectRecord(search->second);
        esm.setKey(ids.key_id);
        esm.setValue(ids.val_id);
        esm_ext.selectRecord(patterns_ext[i].pos);
        esm_ext.setKey(ids.key_id);
        esm_ext.setValue(ids.val_id);
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
                const auto & key_text = esm_ext.getKey().text + "^" + message_ext.at(k);
                const auto & val_text = esm.getKey().text + "^" + message.at(k);
                insertRecordToDict(key_text, val_text, ids.type);
            }
        }
    }
    printLogLine(ids.type);
}

//----------------------------------------------------------
DictCreator::PatternsExt DictCreator::makeDict_Unordered_PatternsExt(const IDs & ids)
{
    PatternsExt patterns_ext;
    for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
    {
        esm_ext.selectRecord(i);
        if (esm_ext.getRecord().id != ids.rec_id)
            continue;

        esm_ext.setKey(ids.key_id);
        if (!esm_ext.getKey().exist)
            continue;

        patterns_ext.push_back({ esm_ext.getKey().text, i, false });
    }
    return patterns_ext;
}

//----------------------------------------------------------
DictCreator::Patterns DictCreator::makeDict_Unordered_Patterns(const IDs & ids)
{
    Patterns patterns;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecord().id != ids.rec_id)
            continue;

        esm.setKey(ids.key_id);
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
void DictCreator::insertRecord(const std::string & id, const std::string & original, Tools::RecType type)
{
    counter_all++;

    Tools::RecordEntry entry;
    entry.id = id;
    entry.original = original;

    if (base_dict)
    {
        auto it = base_dict->find(type);
        if (it != base_dict->end())
        {
            const auto * base_entry = it->second.find(id);
            if (base_entry != nullptr)
            {
                if (base_entry->original == original)
                {
                    entry.translation = base_entry->translation;
                    if (original == base_entry->translation)
                        entry.status = Tools::Status::auto_identical;
                    else
                        entry.status = Tools::Status::translated;
                }
                else
                {
                    entry.translation = base_entry->translation;
                    entry.status = Tools::Status::changed;
                }
            }
            else
            {
                entry.translation = "";
                entry.status = Tools::Status::untranslated;
            }
        }
        else
        {
            entry.translation = "";
            entry.status = Tools::Status::untranslated;
        }
    }
    else
    {
        entry.translation = "";
        entry.status = Tools::Status::untranslated;
    }

    if (dict.at(type).insert(entry))
    {
        counter_created++;
    }
    else
    {
        counter_identical++;
    }
}

//----------------------------------------------------------
void DictCreator::insertRecordToDict(const std::string & id, const std::string & text, Tools::RecType type)
{
    counter_all++;

    Tools::RecordEntry entry;
    entry.id = id;
    entry.original = id;
    entry.translation = text;
    entry.status = "";

    if (dict.at(type).insert(entry))
    {
        counter_created++;
    }
    else
    {
        auto * existing = dict.at(type).find(id);
        if (existing != nullptr && existing->translation != text)
        {
            Tools::RecordEntry doubled_entry;
            doubled_entry.id = id + "^DOUBLED_" + std::to_string(counter_doubled);
            doubled_entry.original = id;
            doubled_entry.translation = text;
            doubled_entry.status = "";
            dict.at(type).insert(doubled_entry);
            counter_doubled++;
            counter_created++;
            if (type != Tools::RecType::Glossary)
                Tools::addLog("Warning: Doubled " + Tools::type2Str(type) + ": " + id + "\r\n");
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
    std::string type_str = Tools::type2Str(type);
    type_str.resize(12, ' ');

    std::ostringstream ss;
    ss << type_str << std::setw(5) << std::to_string(counter_created) << " / ";

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
            if (keyword_pos == std::string::npos)
                continue;
            if (keyword_pos > 0 &&
                (std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos - 1])) ||
                 line_lc[keyword_pos - 1] == '_'))
                continue;
            if (keyword_pos + keyword.size() < line_lc.size() &&
                (std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos + keyword.size()])) ||
                 line_lc[keyword_pos + keyword.size()] == '_'))
                continue;
            keyword_pos_coll.insert(keyword_pos);
        }

        if (keyword_pos_coll.empty())
            continue;

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
