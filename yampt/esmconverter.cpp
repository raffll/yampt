#include "esmconverter.hpp"
#include "scriptparser_ex.hpp"
#include "esmtools.hpp"

//----------------------------------------------------------
EsmConverter::EsmConverter(
    const std::string & path,
    const DictMerger & merger,
    const bool add_hyperlinks,
    const bool safe,
    const std::string & file_suffix,
    const Tools::Encoding encoding
)
    : esm(path)
    , merger(&merger)
    , add_hyperlinks(add_hyperlinks)
    , file_suffix(file_suffix)
{
    if (encoding == Tools::Encoding::WINDOWS_1250)
    {
        esm_encoding = esm.detectEncoding();
        if (esm_encoding == Tools::Encoding::WINDOWS_1250)
        {
            this->add_hyperlinks = false;
        }
    }

    if (esm.isLoaded())
        convertEsm(safe);
}

//----------------------------------------------------------
void EsmConverter::convertEsm(const bool safe)
{
    Tools::addLog("------------------------------------------------\r\n"
                  "      Converted / Identical / Unchanged /    All\r\n"
                  "------------------------------------------------\r\n");

    convertMAST();
    convertCELL();
    convertPGRD();
    convertANAM();
    convertSCVR();
    convertDNAM();
    convertCNDT();
    convertDIAL();
    convertBNAM();
    convertSCPT();

    if (!safe)
    {
        convertGMST();
        convertFNAM();
        convertDESC();
        convertTEXT();
        convertRNAM();
        convertINDX();

        if (add_hyperlinks)
        {
            Tools::addLog("Adding hyperlinks...\r\n");
        }

        convertINFO();
    }

    Tools::addLog("------------------------------------------------\r\n");
}

//----------------------------------------------------------
void EsmConverter::printLogLine(const Tools::RecType type)
{
    std::ostringstream ss;
    ss
        << Tools::getTypeName(type) << " "
        << std::setw(10) << std::to_string(counter_converted) << " / "
        << std::setw(9) << std::to_string(counter_identical) << " / "
        << std::setw(9) << std::to_string(counter_unchanged) << " / "
        << std::setw(6) << std::to_string(counter_all) << std::endl;

    Tools::addLog(ss.str());
}

//----------------------------------------------------------
void EsmConverter::resetCounters()
{
    counter_converted = 0;
    counter_identical = 0;
    counter_unchanged = 0;
    counter_all = 0;
    counter_added = 0;
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent(const std::string & new_friendly)
{
    size_t rec_size;
    std::string rec_content = esm.getRecordContent();
    rec_content.erase(esm.getValue().pos + 8, esm.getValue().size);
    rec_content.insert(esm.getValue().pos + 8, new_friendly);
    rec_content.erase(esm.getValue().pos + 4, 4);
    rec_content.insert(esm.getValue().pos + 4,
                       Tools::convertUIntToStringByteArray(new_friendly.size()));
    rec_size = rec_content.size() - 16;
    rec_content.erase(4, 4);
    rec_content.insert(4, Tools::convertUIntToStringByteArray(rec_size));
    esm.replaceRecord(rec_content);
}

//----------------------------------------------------------
std::string EsmConverter::addNullTerminatorIfEmpty(const std::string & new_friendly)
{
    std::string result;
    if (new_friendly == "")
    {
        result = '\0';
    }
    else
    {
        result = new_friendly;
    }
    return result;
}

//----------------------------------------------------------
std::string EsmConverter::setNewFriendly(
    const Tools::RecType type,
    const std::string & key_text,
    const std::string & val_text,
    const std::string & dialog_topic)
{
    counter_all++;
    std::string new_friendly;
    auto search = merger->getDict().at(type).find(key_text);
    if (search != merger->getDict().at(type).end())
    {
        new_friendly = search->second;
        checkIfIdentical(type, val_text, new_friendly);
    }
    else if (type == Tools::RecType::INFO &&
             add_hyperlinks &&
             dialog_topic.substr(0, 1) != "V")
    {
        new_friendly = Tools::addHyperlinks(
            merger->getDict().at(Tools::RecType::DIAL),
            val_text,
            false);

        checkIfIdentical(type, val_text, new_friendly);

        if (new_friendly.size() > 1024)
        {
            new_friendly.resize(1024);
        }
    }
    else
    {
        to_convert = false;
        counter_unchanged++;
    }
    return new_friendly;
}

//----------------------------------------------------------
std::pair<std::string, std::string> EsmConverter::setNewScript(
    const Tools::RecType type,
    const std::string & script_name,
    const std::string & val_text,
    const std::string & compiled_data)
{
    counter_all++;
    std::string new_friendly;
    std::string new_compiled;
    ScriptParser parser(
        type,
        *merger,
        script_name,
        getNameFull(),
        val_text,
        compiled_data);
    new_friendly = parser.getNewFriendly();
    new_compiled = parser.getNewCompiled();
    checkIfIdentical(type, val_text, new_friendly);
    return make_pair(new_friendly, new_compiled);
}

//----------------------------------------------------------
void EsmConverter::checkIfIdentical(
    const Tools::RecType type,
    const std::string & val_text,
    const std::string & new_friendly)
{
    if (new_friendly != val_text)
    {
        to_convert = true;
        counter_converted++;
    }
    else
    {
        to_convert = false;
        counter_identical++;
    }
}

//----------------------------------------------------------
void EsmConverter::convertMAST()
{
    std::string master_prefix;
    std::string master_suffix;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "TES3")
            continue;

        esm.setValue("MAST");
        while (esm.getValue().exist)
        {
            master_prefix = esm.getValue().text.substr(0, esm.getValue().text.find_last_of("."));
            master_suffix = esm.getValue().text.substr(esm.getValue().text.rfind("."));
            convertRecordContent(master_prefix + file_suffix + master_suffix + '\0');
            esm.setNextValue("MAST");
        }

    }
}

//----------------------------------------------------------
void EsmConverter::convertCELL()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "CELL")
        {
            esm.setValue("NAME");
            if (esm.getValue().exist &&
                esm.getValue().text != "")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getValue().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    // Null terminated
                    // Can't be empty
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void EsmConverter::convertPGRD()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "PGRD")
        {
            esm.setValue("NAME");
            if (esm.getValue().exist &&
                esm.getValue().text != "")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getValue().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(Tools::RecType::PGRD);
}

//----------------------------------------------------------
void EsmConverter::convertANAM()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "INFO")
        {
            esm.setValue("ANAM");
            if (esm.getValue().exist &&
                esm.getValue().text != "")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getValue().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(Tools::RecType::ANAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCVR()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "INFO")
        {
            esm.setValue("SCVR");
            while (esm.getValue().exist)
            {
                if (esm.getValue().text.substr(1, 1) == "B")
                {
                    new_friendly = setNewFriendly(
                        Tools::RecType::CELL,
                        esm.getValue().text.substr(5),
                        esm.getValue().text.substr(5));
                    new_friendly = esm.getValue().text.substr(0, 5) + new_friendly;

                    if (to_convert)
                    {
                        // Not null terminated
                        convertRecordContent(new_friendly);
                    }
                }
                esm.setNextValue("SCVR");
            }
        }
    }
    printLogLine(Tools::RecType::SCVR);
}

//----------------------------------------------------------
void EsmConverter::convertDNAM()
{
    std::string new_friendly;

    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "CELL" ||
            esm.getRecordId() == "NPC_")
        {
            esm.setValue("DNAM");
            while (esm.getValue().exist)
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getValue().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    convertRecordContent(new_friendly + '\0');
                }
                esm.setNextValue("DNAM");
            }
        }
    }
    printLogLine(Tools::RecType::DNAM);
}

//----------------------------------------------------------
void EsmConverter::convertCNDT()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "NPC_")
        {
            esm.setValue("CNDT");
            while (esm.getValue().exist)
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getValue().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    convertRecordContent(new_friendly + '\0');
                }
                esm.setNextValue("CNDT");
            }
        }
    }
    printLogLine(Tools::RecType::CNDT);
}

//----------------------------------------------------------
void EsmConverter::convertGMST()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "GMST")
        {
            esm.setKey("NAME");
            esm.setValue("STRV");
            if (esm.getKey().exist &&
                esm.getValue().exist &&
                esm.getKey().text.substr(0, 1) == "s")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::GMST,
                    esm.getKey().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    // Null terminated only if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
                }
            }
        }
    }
    printLogLine(Tools::RecType::GMST);
}

//----------------------------------------------------------
void EsmConverter::convertFNAM()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "ACTI" || esm.getRecordId() == "ALCH" ||
            esm.getRecordId() == "APPA" || esm.getRecordId() == "ARMO" ||
            esm.getRecordId() == "BOOK" || esm.getRecordId() == "BSGN" ||
            esm.getRecordId() == "CLAS" || esm.getRecordId() == "CLOT" ||
            esm.getRecordId() == "CONT" || esm.getRecordId() == "CREA" ||
            esm.getRecordId() == "DOOR" || esm.getRecordId() == "FACT" ||
            esm.getRecordId() == "INGR" || esm.getRecordId() == "LIGH" ||
            esm.getRecordId() == "LOCK" || esm.getRecordId() == "MISC" ||
            esm.getRecordId() == "NPC_" || esm.getRecordId() == "PROB" ||
            esm.getRecordId() == "RACE" || esm.getRecordId() == "REGN" ||
            esm.getRecordId() == "REPA" || esm.getRecordId() == "SKIL" ||
            esm.getRecordId() == "SPEL" || esm.getRecordId() == "WEAP")
        {
            esm.setKey("NAME");
            esm.setValue("FNAM");
            if (esm.getKey().exist &&
                esm.getValue().exist &&
                esm.getKey().text != "player")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::FNAM,
                    esm.getRecordId() + Tools::sep[0] + esm.getKey().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    // Null terminated
                    // Don't exist if empty
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(Tools::RecType::FNAM);
}

//----------------------------------------------------------
void EsmConverter::convertDESC()
{
    std::string new_friendly;
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
                new_friendly = setNewFriendly(
                    Tools::RecType::DESC,
                    esm.getRecordId() + Tools::sep[0] + esm.getKey().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    if (esm.getRecordId() == "BSGN")
                    {
                        // Null terminated
                        // Don't exist if empty
                        convertRecordContent(new_friendly + '\0');
                    }
                    if (esm.getRecordId() == "CLAS" ||
                        esm.getRecordId() == "RACE")
                    {
                        // Not null terminated
                        // Don't exist if empty
                        convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
                    }
                }
            }
        }
    }
    printLogLine(Tools::RecType::DESC);
}

//----------------------------------------------------------
void EsmConverter::convertTEXT()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "BOOK")
        {
            esm.setKey("NAME");
            esm.setValue("TEXT");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::TEXT,
                    esm.getKey().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    // Not null terminated
                    // Don't exist if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
                }
            }
        }
    }
    printLogLine(Tools::RecType::TEXT);
}

//----------------------------------------------------------
void EsmConverter::convertRNAM()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "FACT")
        {
            esm.setKey("NAME");
            esm.setValue("RNAM");
            if (esm.getKey().exist)
            {
                while (esm.getValue().exist)
                {
                    new_friendly = setNewFriendly(
                        Tools::RecType::RNAM,
                        esm.getKey().text + Tools::sep[0] + std::to_string(esm.getValue().counter),
                        esm.getValue().text);

                    if (to_convert)
                    {
                        // Null terminated up to 32
                        new_friendly.resize(32);
                        convertRecordContent(new_friendly);
                    }
                    esm.setNextValue("RNAM");
                }
            }
        }
    }
    printLogLine(Tools::RecType::RNAM);
}

//----------------------------------------------------------
void EsmConverter::convertINDX()
{
    std::string new_friendly;
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
                new_friendly = setNewFriendly(
                    Tools::RecType::INDX,
                    esm.getRecordId() + Tools::sep[0] + esm.getKey().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    // Not null terminated
                    // Don't exist if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
                }
            }
        }
    }
    printLogLine(Tools::RecType::INDX);
}

//----------------------------------------------------------
void EsmConverter::convertDIAL()
{
    std::string new_friendly;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "DIAL")
        {
            esm.setKey("DATA");
            esm.setValue("NAME");
            if (esm.getKey().exist &&
                esm.getValue().exist &&
                esm.getKey().text == "T")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::DIAL,
                    esm.getValue().text,
                    esm.getValue().text);

                if (to_convert)
                {
                    // Null terminated
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
    std::string new_friendly;
    std::string dialog_topic;
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
                dialog_topic = esm.getKey().text + Tools::sep[0] + esm.getValue().text;
            }
        }
        if (esm.getRecordId() == "INFO")
        {
            esm.setKey("INAM");
            esm.setValue("NAME");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::INFO,
                    dialog_topic + Tools::sep[0] + esm.getKey().text,
                    esm.getValue().text,
                    dialog_topic);

                if (to_convert)
                {
                    // Not null terminated
                    // Don't exist if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
                }
            }
        }
    }
    printLogLine(Tools::RecType::INFO);
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
    std::pair<std::string, std::string> new_script;
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "INFO")
        {
            esm.setKey("INAM");
            esm.setValue("BNAM");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                new_script = setNewScript(
                    Tools::RecType::BNAM,
                    esm.getKey().text,
                    esm.getValue().text,
                    "");

                if (to_convert)
                {
                    convertRecordContent(new_script.first);
                }
            }
        }
    }
    Tools::addLog("---\r\n", true);
    printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCPT()
{
    std::string compiled_data;
    std::pair<std::string, std::string> new_script;
    std::string new_header;

    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "SCPT")
        {
            esm.setKey("SCHD");
            esm.setValue("SCDT");
            if (esm.getValue().exist)
            {
                compiled_data = esm.getValue().content;
            }
            else
            {
                compiled_data.clear();
            }
            esm.setValue("SCTX");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                new_script = setNewScript(
                    Tools::RecType::SCTX,
                    esm.getKey().text,
                    esm.getValue().text,
                    compiled_data);

                if (to_convert)
                {
                    esm.setValue("SCTX");
                    convertRecordContent(new_script.first);
                    esm.setValue("SCDT");
                    convertRecordContent(new_script.second);

                    // Compiled script data size in script name
                    esm.setValue("SCHD");
                    new_header = esm.getValue().content;
                    new_header.erase(44, 4);
                    new_header.insert(44, Tools::convertUIntToStringByteArray(new_script.second.size()));
                    convertRecordContent(new_header);
                }
            }
        }
    }
    Tools::addLog("---\r\n", true);
    printLogLine(Tools::RecType::SCTX);
}
