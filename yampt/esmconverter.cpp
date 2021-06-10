#include "esmconverter.hpp"
#include "scriptparser.hpp"

//----------------------------------------------------------
EsmConverter::EsmConverter(
    const std::string & path,
    const DictMerger & merger,
    const bool add_hyperlinks,
    const std::string & file_suffix,
    const Tools::Encoding encoding
)
    : esm(path)
    , merger(merger)
    , add_hyperlinks(add_hyperlinks)
    , file_suffix(file_suffix)
{
    if (encoding == Tools::Encoding::WINDOWS_1250)
    {
        esm_encoding = detectEncoding();
        if (esm_encoding == Tools::Encoding::WINDOWS_1250)
        {
            this->add_hyperlinks = false;
        }
    }

    if (esm.isLoaded())
        convertEsm();
}

//----------------------------------------------------------
void EsmConverter::convertEsm()
{
    Tools::addLog(
        "------------------------------------------------\r\n"
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
    convertGMST();
    convertFNAM();
    convertDESC();
    convertTEXT();
    convertRNAM();
    convertINDX();
    convertGMDT();

    if (add_hyperlinks)
    {
        Tools::addLog("Adding hyperlinks...\r\n");
    }

    convertINFO();

    Tools::addLog("------------------------------------------------\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertMAST()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "TES3")
            continue;

        esm.setValue("MAST");
        while (esm.getValue().exist)
        {
            const auto & prefix = esm.getValue().text.substr(0, esm.getValue().text.find_last_of("."));
            const auto & suffix = esm.getValue().text.substr(esm.getValue().text.rfind("."));
            const auto & new_text = prefix + file_suffix + suffix + '\0';
            convertRecordContent(new_text);
            esm.setNextValue("MAST");
        }
    }
}

//----------------------------------------------------------
void EsmConverter::convertGMDT()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "TES3")
        {
            esm.setValue("GMDT");
            if (esm.getValue().exist)
            {
                const auto & prefix = esm.getValue().content.substr(0, 24);
                const auto & suffix = esm.getValue().content.substr(88);
                auto val_text = esm.getValue().content.substr(24, 64);
                val_text = Tools::eraseNullChars(val_text);
                const auto & type = Tools::RecType::CELL;
                std::string new_text;
                if (!makeNewText({ val_text, val_text, type }, new_text))
                    continue;

                new_text.resize(64);
                convertRecordContent(prefix + new_text + suffix);
            }
        }

        if (esm.getRecordId() == "GAME")
        {
            esm.setValue("GMDT");
            if (esm.getValue().exist)
            {
                const auto & suffix = esm.getValue().content.substr(64);
                auto val_text = esm.getValue().content.substr(0, 64);
                val_text = Tools::eraseNullChars(val_text);
                const auto & type = Tools::RecType::CELL;
                std::string new_text;
                if (!makeNewText({ val_text, val_text, type }, new_text))
                    continue;

                new_text.resize(64);
                convertRecordContent(new_text + suffix);
            }
        }
    }
    printLogLine(Tools::RecType::GMDT);
}

//----------------------------------------------------------
void EsmConverter::convertCELL()
{
    resetCounters();
    const auto & type = Tools::RecType::CELL;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "CELL")
            continue;

        esm.setValue("NAME");
        if (esm.getValue().exist &&
            esm.getValue().text != "")
        {
            const auto & key_text = esm.getValue().text;
            const auto & val_text = esm.getValue().text;
            std::string new_text;
            if (!makeNewText({ key_text, val_text, type }, new_text))
                continue;

            /* null terminated, can't be empty */
            new_text += '\0';
            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void EsmConverter::convertPGRD()
{
    resetCounters();
    const auto & type = Tools::RecType::CELL;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "PGRD")
            continue;

        esm.setValue("NAME");
        if (esm.getValue().exist &&
            esm.getValue().text != "")
        {
            const auto & key_text = esm.getValue().text;
            const auto & val_text = esm.getValue().text;
            std::string new_text;
            if (!makeNewText({ key_text, val_text, type }, new_text))
                continue;

            new_text += '\0';
            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::PGRD);
}

//----------------------------------------------------------
void EsmConverter::convertANAM()
{
    resetCounters();
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "INFO")
            continue;

        esm.setValue("ANAM");
        if (esm.getValue().exist &&
            esm.getValue().text != "")
        {
            const auto & key_text = esm.getValue().text;
            const auto & val_text = esm.getValue().text;
            const auto & type = Tools::RecType::CELL;
            std::string new_text;
            if (!makeNewText({ key_text, val_text, type }, new_text))
                continue;

            new_text += '\0';
            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::ANAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCVR()
{
    resetCounters();
    const auto & type = Tools::RecType::CELL;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "INFO")
            continue;

        esm.setValue("SCVR");
        while (esm.getValue().exist)
        {
            /* possible exceptions */
            if (esm.getValue().text.substr(1, 1) == "B")
            {
                const auto & key_text = esm.getValue().text.substr(5);
                const auto & val_text = esm.getValue().text.substr(5);
                std::string new_text;
                if (makeNewText({ key_text, val_text, type }, new_text))
                {
                    /* not null terminated */
                    new_text = esm.getValue().text.substr(0, 5) + new_text;
                    convertRecordContent(new_text);
                }
            }
            esm.setNextValue("SCVR");
        }
    }
    printLogLine(Tools::RecType::SCVR);
}

//----------------------------------------------------------
void EsmConverter::convertDNAM()
{
    resetCounters();
    const auto & type = Tools::RecType::CELL;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "CELL" ||
            esm.getRecordId() == "NPC_")
        {
            esm.setValue("DNAM");
            while (esm.getValue().exist)
            {
                const auto & key_text = esm.getValue().text;
                const auto & val_text = esm.getValue().text;
                std::string new_text;
                if (makeNewText({ key_text, val_text, type }, new_text))
                {
                    new_text += '\0';
                    convertRecordContent(new_text);
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
    resetCounters();
    const auto & type = Tools::RecType::CELL;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "NPC_")
            continue;

        esm.setValue("CNDT");
        while (esm.getValue().exist)
        {
            const auto & key_text = esm.getValue().text;
            const auto & val_text = esm.getValue().text;
            std::string new_text;
            if (makeNewText({ key_text, val_text, type }, new_text))
            {
                new_text += '\0';
                convertRecordContent(new_text);
            }
            esm.setNextValue("CNDT");
        }
    }
    printLogLine(Tools::RecType::CNDT);
}

//----------------------------------------------------------
void EsmConverter::convertGMST()
{
    resetCounters();
    const auto & type = Tools::RecType::GMST;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "GMST")
            continue;

        esm.setKey("NAME");
        esm.setValue("STRV");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm.getKey().text.substr(0, 1) == "s") /* possible exception */
        {
            const auto & key_text = esm.getKey().text;
            const auto & val_text = esm.getValue().text;
            std::string new_text;
            if (!makeNewText({ key_text, val_text, type }, new_text))
                continue;

            /* null terminated only if empty */
            addNullTerminatorIfEmpty(new_text);
            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::GMST);
}

//----------------------------------------------------------
void EsmConverter::convertFNAM()
{
    resetCounters();
    const auto & type = Tools::RecType::FNAM;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (!Tools::isFNAM(esm.getRecordId()))
            continue;

        esm.setKey("NAME");
        esm.setValue("FNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist &&
            esm.getKey().text != "player")
        {
            const auto & key_text = esm.getRecordId() + Tools::sep[0] + esm.getKey().text;
            const auto & val_text = esm.getValue().text;
            std::string new_text;
            if (!makeNewText({ key_text, val_text, type }, new_text))
                continue;

            /* null terminated, don't exist if empty */
            new_text += '\0';
            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::FNAM);
}

//----------------------------------------------------------
void EsmConverter::convertDESC()
{
    resetCounters();
    const auto & type = Tools::RecType::DESC;
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
                std::string new_text;
                if (!makeNewText({ key_text, val_text, type }, new_text))
                    continue;

                if (esm.getRecordId() == "BSGN")
                {
                    /* null terminated, don't exist if empty */
                    new_text += '\0';
                    convertRecordContent(new_text);
                }

                if (esm.getRecordId() == "CLAS" ||
                    esm.getRecordId() == "RACE")
                {
                    /* not null terminated, don't exist if empty */
                    addNullTerminatorIfEmpty(new_text);
                    convertRecordContent(new_text);
                }
            }
        }
    }
    printLogLine(Tools::RecType::DESC);
}

//----------------------------------------------------------
void EsmConverter::convertTEXT()
{
    resetCounters();
    const auto & type = Tools::RecType::TEXT;
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
            std::string new_text;
            if (!makeNewText({ key_text, val_text, type }, new_text))
                continue;

            /* not null terminated, don't exist if empty */
            addNullTerminatorIfEmpty(new_text);
            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::TEXT);
}

//----------------------------------------------------------
void EsmConverter::convertRNAM()
{
    resetCounters();
    const auto & type = Tools::RecType::RNAM;
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
            std::string new_text;
            if (makeNewText({ key_text, val_text, type }, new_text))
            {
                /* null terminated up to 32 */
                new_text.resize(32);
                convertRecordContent(new_text);
            }
            esm.setNextValue("RNAM");
        }
    }
    printLogLine(Tools::RecType::RNAM);
}

//----------------------------------------------------------
void EsmConverter::convertINDX()
{
    resetCounters();
    const auto & type = Tools::RecType::INDX;
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
                std::string new_text;
                if (!makeNewText({ key_text, val_text, type }, new_text))
                    continue;

                /* not null terminated, don't exist if empty */
                addNullTerminatorIfEmpty(new_text);
                convertRecordContent(new_text);
            }
        }
    }
    printLogLine(Tools::RecType::INDX);
}

//----------------------------------------------------------
void EsmConverter::convertDIAL()
{
    resetCounters();
    const auto & type = Tools::RecType::DIAL;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "DIAL")
            continue;

        esm.setKey("DATA");
        esm.setValue("NAME");
        if (Tools::getDialogType(esm.getKey().content) == "T" &&
            esm.getValue().exist)
        {
            const auto & key_text = esm.getValue().text;
            const auto & val_text = esm.getValue().text;
            std::string new_text;
            if (!makeNewText({ key_text, val_text, type }, new_text))
                continue;

            /* null terminated */
            new_text += '\0';
            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
    std::string key_prefix;
    resetCounters();
    const auto & type = Tools::RecType::INFO;
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
                key_prefix = Tools::getDialogType(esm.getKey().content) + Tools::sep[0] + esm.getValue().text;
            }
        }

        if (esm.getRecordId() == "INFO")
        {
            esm.setKey("INAM");
            esm.setValue("NAME");
            if (esm.getKey().exist &&
                esm.getValue().exist)
            {
                const auto & key_text = key_prefix + Tools::sep[0] + esm.getKey().text;
                const auto & val_text = esm.getValue().text;
                std::string new_text;
                if (makeNewText({ key_text, val_text, type }, new_text))
                {
                    /* not null terminated, don't exist if empty */
                    addNullTerminatorIfEmpty(new_text);
                    convertRecordContent(new_text);
                }
            }
        }
    }
    printLogLine(Tools::RecType::INFO);
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
    resetCounters();
    const auto & type = Tools::RecType::BNAM;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "INFO")
            continue;

        esm.setKey("INAM");
        esm.setValue("BNAM");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            const auto & key_text = esm.getKey().text;
            const auto & val_text = esm.getValue().text;

            const auto & script_name = key_text;
            const auto & file_name = getName().full;
            const auto & old_script = val_text;

            counter_all++;
            ScriptParser parser(
                type,
                merger,
                script_name,
                file_name,
                old_script);

            std::string new_text = parser.getNewScript();
            if (isIdentical(val_text, new_text))
                continue;

            convertRecordContent(new_text);
        }
    }
    printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCPT()
{
    std::string old_SCDT;
    resetCounters();
    const auto & type = Tools::RecType::SCTX;
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() != "SCPT")
            continue;

        esm.setValue("SCDT");
        if (esm.getValue().exist)
        {
            old_SCDT = esm.getValue().content;
        }
        else
        {
            old_SCDT.clear();
        }

        esm.setKey("SCHD");
        esm.setValue("SCTX");
        if (esm.getKey().exist &&
            esm.getValue().exist)
        {
            const auto & key_text = esm.getKey().text;
            const auto & val_text = esm.getValue().text;

            const auto & script_name = key_text;
            const auto & file_name = getName().full;
            const auto & old_script = val_text;

            counter_all++;
            ScriptParser parser(
                type,
                merger,
                script_name,
                file_name,
                old_script,
                old_SCDT);

            const auto & new_text = parser.getNewScript();
            if (isIdentical(val_text, new_text))
                continue;

            convertRecordContent(new_text);

            {
                /* compiled script data */
                esm.setValue("SCDT");
                const auto & new_text = parser.getNewSCDT();
                convertRecordContent(new_text);
            }

            {
                /* compiled script data size in script name */
                esm.setValue("SCHD");
                auto new_text = esm.getValue().content;
                new_text.erase(44, 4);
                new_text.insert(44, Tools::convertUIntToStringByteArray(parser.getNewSCDT().size()));
                convertRecordContent(new_text);
            }
        }
    }
    printLogLine(Tools::RecType::SCTX);
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
bool EsmConverter::makeNewText(
    const Tools::Entry & entry,
    std::string & new_text)
{
    counter_all++;
    new_text.clear();
    auto search = merger.getDict().at(entry.type).find(entry.key_text);
    if (search != merger.getDict().at(entry.type).end())
    {
        new_text = search->second;
        return !isIdentical(entry.val_text, new_text);
    }
    else if (
        entry.type == Tools::RecType::INFO &&
        add_hyperlinks &&
        entry.optional.substr(0, 1) != "V")
    {
        new_text = entry.val_text + Tools::addHyperlinks(
            merger.getDict().at(Tools::RecType::DIAL),
            entry.val_text,
            false);

        if (new_text.size() > 1024)
            new_text.resize(1024);

        return !isIdentical(entry.val_text, new_text);
    }

    counter_unchanged++;
    return false;
}

//----------------------------------------------------------
bool EsmConverter::isIdentical(
    const std::string & old_text,
    const std::string & new_text)
{
    if (old_text == new_text)
    {
        counter_identical++;
        return true;
    }

    return false;
}

//----------------------------------------------------------
void EsmConverter::addNullTerminatorIfEmpty(
    std::string & new_text)
{
    if (new_text.empty())
        new_text = '\0';
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent(const std::string & new_text)
{
    size_t rec_size;
    std::string rec_content = esm.getRecordContent();
    rec_content.erase(esm.getValue().pos + 8, esm.getValue().size);
    rec_content.insert(esm.getValue().pos + 8, new_text);
    rec_content.erase(esm.getValue().pos + 4, 4);
    rec_content.insert(
        esm.getValue().pos + 4,
        Tools::convertUIntToStringByteArray(new_text.size()));
    rec_size = rec_content.size() - 16;
    rec_content.erase(4, 4);
    rec_content.insert(4, Tools::convertUIntToStringByteArray(rec_size));
    esm.replaceRecord(rec_content);
    counter_converted++;
}

//----------------------------------------------------------
void EsmConverter::printLogLine(const Tools::RecType type)
{
    std::ostringstream ss;
    ss
        << Tools::type2Str(type) << " "
        << std::setw(10) << std::to_string(counter_converted) << " / "
        << std::setw(9) << std::to_string(counter_identical) << " / "
        << std::setw(9) << std::to_string(counter_unchanged) << " / "
        << std::setw(6) << std::to_string(counter_all) << std::endl;

    Tools::addLog(ss.str());
}

//----------------------------------------------------------
Tools::Encoding EsmConverter::detectEncoding()
{
    for (size_t i = 0; i < esm.getRecords().size(); ++i)
    {
        esm.selectRecord(i);
        if (esm.getRecordId() == "INFO")
            esm.setValue("NAME");

        if (detectWindows1250Encoding(esm.getValue().text))
        {
            Tools::addLog("--> Windows-1250 encoding detected!\r\n");
            Tools::addLog("INFO: " + esm.getValue().text + "\r\n", true);
            return Tools::Encoding::WINDOWS_1250;
        }
    }
    return Tools::Encoding::UNKNOWN;
}

//----------------------------------------------------------
bool EsmConverter::detectWindows1250Encoding(const std::string & val_text)
{
    // 156 œ ś
    // 159 Ÿ ź
    // 179 ³ ł
    // 185 ¹ ą
    // 191 ¿ ż
    // 230 æ ć
    // 234 ê ę
    // 241 ñ ń
    // 243 ó ó <- found in Tamriel Rebuilt

    std::ostringstream ss;
    ss
        << static_cast<char>(156)
        << static_cast<char>(159)
        << static_cast<char>(179)
        << static_cast<char>(185)
        << static_cast<char>(191)
        << static_cast<char>(230)
        << static_cast<char>(234)
        << static_cast<char>(241);

    return val_text.find_first_of(ss.str()) != std::string::npos;
}
