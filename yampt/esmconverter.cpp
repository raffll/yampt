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
    Tools::addLog("----------------------------------------------\r\n"
                  "      Converted / Identical / Unchanged /    All\r\n"
                  "----------------------------------------------\r\n");

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
            Tools::addLog("---\r\n", true);
            Tools::addLog("Adding hyperlinks...\r\n");
        }

        convertINFO();
    }

    Tools::addLog("----------------------------------------------\r\n");
}

//----------------------------------------------------------
void EsmConverter::printLogLine(const Tools::RecType type)
{
    std::ostringstream ss;
    ss
        << Tools::type_name[type] << " "
        << std::setw(10) << std::to_string(counter_converted) << " / "
        << std::setw(9) << std::to_string(counter_identical) << " / "
        << std::setw(9) << std::to_string(counter_unchanged) << " / "
        << std::setw(6) << std::to_string(counter_all) << std::endl;

    Tools::addLog("---\r\n", true);
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
    rec_content.erase(esm.getFriendlyPos() + 8, esm.getFriendlySize());
    rec_content.insert(esm.getFriendlyPos() + 8, new_friendly);
    rec_content.erase(esm.getFriendlyPos() + 4, 4);
    rec_content.insert(esm.getFriendlyPos() + 4,
                       Tools::convertUIntToStringByteArray(new_friendly.size()));
    rec_size = rec_content.size() - 16;
    rec_content.erase(4, 4);
    rec_content.insert(4, Tools::convertUIntToStringByteArray(rec_size));
    esm.replaceRecordContent(rec_content);
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
    const std::string & unique_text,
    const std::string & friendly_text,
    const std::string & dialog_topic)
{
    counter_all++;
    std::string new_friendly;
    auto search = merger->getDict(type).find(unique_text);
    if (search != merger->getDict(type).end())
    {
        new_friendly = search->second;
        checkIfIdentical(type, friendly_text, new_friendly);
    }
    else if (type == Tools::RecType::INFO &&
             add_hyperlinks &&
             dialog_topic.substr(0, 1) != "V")
    {
        new_friendly = Tools::addHyperlinks(
            merger->getDict(Tools::RecType::DIAL),
            friendly_text,
            false);

        checkIfIdentical(type, friendly_text, new_friendly);

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
    const std::string & line_prefix,
    const std::string & friendly_text,
    const std::string & compiled_data)
{
    counter_all++;
    std::string new_friendly;
    std::string new_compiled;
    ScriptParser parser(
        type,
        *merger,
        line_prefix,
        friendly_text,
        compiled_data);
    new_friendly = parser.getNewFriendly();
    new_compiled = parser.getNewCompiled();
    checkIfIdentical(type, friendly_text, new_friendly);
    return make_pair(new_friendly, new_compiled);
}

//----------------------------------------------------------
void EsmConverter::checkIfIdentical(
    const Tools::RecType type,
    const std::string & friendly_text,
    const std::string & new_friendly)
{
    if (new_friendly != friendly_text)
    {
        to_convert = true;

        if (type == Tools::RecType::GMST ||
            type == Tools::RecType::FNAM ||
            type == Tools::RecType::DESC ||
            //type == Tools::RecType::TEXT ||
            type == Tools::RecType::RNAM ||
            type == Tools::RecType::INDX ||
            type == Tools::RecType::INFO)
        {
            Tools::addLog("---\r\n", true);
            Tools::addLog(Tools::type_name[type] + " <<< " + friendly_text + "\r\n", true);
            Tools::addLog(Tools::type_name[type] + " >>> " + new_friendly + "\r\n", true);
        }

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "TES3")
        {
            esm.setFriendlyTo("MAST");
            while (esm.isFriendlyValid())
            {
                master_prefix = esm.getFriendlyText().substr(0, esm.getFriendlyText().find_last_of("."));
                master_suffix = esm.getFriendlyText().substr(esm.getFriendlyText().rfind("."));
                convertRecordContent(master_prefix + file_suffix + master_suffix + '\0');
                esm.setNextFriendlyTo("MAST");
            }
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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "CELL")
        {
            esm.setFriendlyTo("NAME");
            if (esm.isFriendlyValid() &&
                esm.getFriendlyText() != "")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getFriendlyText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "PGRD")
        {
            esm.setFriendlyTo("NAME");
            if (esm.isFriendlyValid() &&
                esm.getFriendlyText() != "")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getFriendlyText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "INFO")
        {
            esm.setFriendlyTo("ANAM");
            if (esm.isFriendlyValid() &&
                esm.getFriendlyText() != "")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getFriendlyText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "INFO")
        {
            esm.setFriendlyTo("SCVR");
            while (esm.isFriendlyValid())
            {
                if (esm.getFriendlyText().substr(1, 1) == "B")
                {
                    new_friendly = setNewFriendly(
                        Tools::RecType::CELL,
                        esm.getFriendlyText().substr(5),
                        esm.getFriendlyText().substr(5));
                    new_friendly = esm.getFriendlyText().substr(0, 5) + new_friendly;

                    if (to_convert)
                    {
                        // Not null terminated
                        convertRecordContent(new_friendly);
                    }
                }
                esm.setNextFriendlyTo("SCVR");
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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "CELL" ||
            esm.getRecordId() == "NPC_")
        {
            esm.setFriendlyTo("DNAM");
            while (esm.isFriendlyValid())
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getFriendlyText(),
                    esm.getFriendlyText());

                if (to_convert)
                {
                    convertRecordContent(new_friendly + '\0');
                }
                esm.setNextFriendlyTo("DNAM");
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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "NPC_")
        {
            esm.setFriendlyTo("CNDT");
            while (esm.isFriendlyValid())
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::CELL,
                    esm.getFriendlyText(),
                    esm.getFriendlyText());

                if (to_convert)
                {
                    convertRecordContent(new_friendly + '\0');
                }
                esm.setNextFriendlyTo("CNDT");
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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "GMST")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("STRV");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid() &&
                esm.getUniqueText().substr(0, 1) == "s")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::GMST,
                    esm.getUniqueText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
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
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("FNAM");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid() &&
                esm.getUniqueText() != "player")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::FNAM,
                    esm.getRecordId() + Tools::sep[0] + esm.getUniqueText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "BSGN" ||
            esm.getRecordId() == "CLAS" ||
            esm.getRecordId() == "RACE")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("DESC");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid())
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::DESC,
                    esm.getRecordId() + Tools::sep[0] + esm.getUniqueText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "BOOK")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("TEXT");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid())
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::TEXT,
                    esm.getUniqueText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "FACT")
        {
            esm.setUniqueTo("NAME");
            esm.setFriendlyTo("RNAM");
            if (esm.isUniqueValid())
            {
                while (esm.isFriendlyValid())
                {
                    new_friendly = setNewFriendly(
                        Tools::RecType::RNAM,
                        esm.getUniqueText() + Tools::sep[0] + esm.getFriendlyCounter(),
                        esm.getFriendlyText());

                    if (to_convert)
                    {
                        // Null terminated up to 32
                        new_friendly.resize(32);
                        convertRecordContent(new_friendly);
                    }
                    esm.setNextFriendlyTo("RNAM");
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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "SKIL" ||
            esm.getRecordId() == "MGEF")
        {
            esm.setUniqueTo("INDX");
            esm.setFriendlyTo("DESC");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid())
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::INDX,
                    esm.getRecordId() + Tools::sep[0] + esm.getUniqueText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "DIAL")
        {
            esm.setUniqueTo("DATA");
            esm.setFriendlyTo("NAME");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid() &&
                esm.getUniqueText() == "T")
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::DIAL,
                    esm.getFriendlyText(),
                    esm.getFriendlyText());

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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "DIAL")
        {
            esm.setUniqueTo("DATA");
            esm.setFriendlyTo("NAME");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid())
            {
                dialog_topic = esm.getUniqueText() + Tools::sep[0] + esm.getFriendlyText();
            }
        }
        if (esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFriendlyTo("NAME");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid())
            {
                new_friendly = setNewFriendly(
                    Tools::RecType::INFO,
                    dialog_topic + Tools::sep[0] + esm.getUniqueText(),
                    esm.getFriendlyText(),
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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFriendlyTo("BNAM");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid())
            {
                new_script = setNewScript(
                    Tools::RecType::BNAM,
                    esm.getUniqueText() + Tools::sep[0],
                    esm.getFriendlyText(),
                    "");

                if (to_convert)
                {
                    convertRecordContent(new_script.first);
                }
            }
        }
    }
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
        esm.setRecordTo(i);
        if (esm.getRecordId() == "SCPT")
        {
            esm.setUniqueTo("SCHD");
            esm.setFriendlyTo("SCDT");
            if (esm.isFriendlyValid())
            {
                compiled_data = esm.getFriendlyWithNull();
            }
            else
            {
                compiled_data.clear();
            }
            esm.setFriendlyTo("SCTX");
            if (esm.isUniqueValid() &&
                esm.isFriendlyValid())
            {
                new_script = setNewScript(
                    Tools::RecType::SCTX,
                    esm.getUniqueText() + Tools::sep[0],
                    esm.getFriendlyText(),
                    compiled_data);

                if (to_convert)
                {
                    esm.setFriendlyTo("SCTX");
                    convertRecordContent(new_script.first);
                    esm.setFriendlyTo("SCDT");
                    convertRecordContent(new_script.second);

                    // Compiled script data size in script name
                    esm.setFriendlyTo("SCHD");
                    new_header = esm.getFriendlyWithNull();
                    new_header.erase(44, 4);
                    new_header.insert(44, Tools::convertUIntToStringByteArray(new_script.second.size()));
                    convertRecordContent(new_header);
                }
            }
        }
    }
    printLogLine(Tools::RecType::SCTX);
}
