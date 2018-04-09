#include "esmconverter.hpp"

//----------------------------------------------------------
EsmConverter::EsmConverter(std::string path,
                           DictMerger &merger,
                           bool add_dial)
    : merger(&merger),
      add_dial(add_dial)
{
    esm.readFile(path);
    if(esm.getStatus() == true &&
       this->merger->getStatus() == true)
    {
        status = true;
    }
}

//----------------------------------------------------------
void EsmConverter::convertEsm()
{
    if(status == true)
    {
        printLogHeader();
        convertCELL();
        convertPGRD();
        convertANAM();
        convertSCVR();
        convertDNAM();
        convertCNDT();
        convertGMST();
        convertFNAM();
        convertDESC();
        convertTEXT();
        convertRNAM();
        convertINDX();
        convertDIAL();
        convertINFO();
        convertBNAM();
        convertSCPT();
        convertGMDT();
        std::cout << "----------------------------------------------" << std::endl;
    }
}

//----------------------------------------------------------
void EsmConverter::printLogHeader()
{
    std::cout << "----------------------------------------------" << std::endl
              << "      Converted / Skipped / Unchanged /    All" << std::endl
              << "----------------------------------------------" << std::endl;
}

//----------------------------------------------------------
void EsmConverter::printLogLine(yampt::rec_type type)
{
    if(type == yampt::rec_type::INFO &&
       add_dial == true)
    {
        std::cout << yampt::type_name[type] << " "
                  << std::setw(10) << std::to_string(counter_converted) << " / "
                  << std::setw(7) << std::to_string(counter_skipped) << " / "
                  << std::setw(9) << std::to_string(counter_unchanged) << " / "
                  << std::setw(6) << std::to_string(counter_all) << std::endl
                  << "+ Link" << " "
                  << std::setw(8) << std::to_string(counter_added) << " / "
                  << std::setw(7) << "-" << " / "
                  << std::setw(9) << "-" << " / "
                  << std::setw(6) << "-" << std::endl;
    }
    else
    {
        std::cout << yampt::type_name[type] << " "
                  << std::setw(10) << std::to_string(counter_converted) << " / "
                  << std::setw(7) << std::to_string(counter_skipped) << " / "
                  << std::setw(9) << std::to_string(counter_unchanged) << " / "
                  << std::setw(6) << std::to_string(counter_all) << std::endl;
    }
}

//----------------------------------------------------------
void EsmConverter::resetCounters()
{
    counter_converted = 0;
    counter_skipped = 0;
    counter_unchanged = 0;
    counter_all = 0;
    counter_added = 0;
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent(const std::string &new_friendly)
{
    try
    {
        size_t rec_size;
        std::string rec_content = esm.getRecordContent();
        rec_content.erase(esm.getFriendlyPos() + 8, esm.getFriendlySize());
        rec_content.insert(esm.getFriendlyPos() + 8, new_friendly);
        rec_content.erase(esm.getFriendlyPos() + 4, 4);
        rec_content.insert(esm.getFriendlyPos() + 4, tools.convertUIntToStringByteArray(new_friendly.size()));
        rec_size = rec_content.size() - 16;
        rec_content.erase(4, 4);
        rec_content.insert(4, tools.convertUIntToStringByteArray(rec_size));
        esm.setNewRecordContent(rec_content);
    }
    catch(std::exception const& e)
    {
        std::cout << "--> Error in function convertRecordContent() (possibly broken file or record)!" << std::endl;
        std::cout << "--> Exception: " << e.what() << std::endl;
        status = false;
    }
}

//----------------------------------------------------------
std::string EsmConverter::setNewFriendly(yampt::rec_type type,
                                         const std::string &unique_text,
                                         const std::string &friendly_text,
                                         const std::string &dialog_topic)
{
    counter_all++;
    std::string new_friendly;
    auto search = merger->getDict(type).find(unique_text);
    if(search != merger->getDict(type).end())
    {
        new_friendly = search->second;
        setToConvertFlag(friendly_text, new_friendly);
    }
    else if(type == yampt::rec_type::INFO &&
            add_dial == true &&
            dialog_topic.substr(0, 1) != "V")
    {
        new_friendly = addDialogTopicsToNotConvertedINFOStrings(friendly_text);
        setToConvertFlag(friendly_text, new_friendly);
    }
    else
    {
        to_convert = false;
        counter_unchanged++;
    }
    return new_friendly;
}

//----------------------------------------------------------
std::string EsmConverter::setNewScriptBNAM(const std::string &friendly_text)
{
    counter_all++;
    std::string new_friendly;
    ScriptParser parser(yampt::rec_type::BNAM, *merger, friendly_text);
    parser.convertScript();
    new_friendly = parser.getNewFriendly();
    setToConvertFlag(friendly_text, new_friendly);
    return new_friendly;
}

//----------------------------------------------------------
std::pair<std::string, std::string> EsmConverter::setNewScriptSCPT(const std::string &friendly_text,
                                                                   const std::string &compiled_data)
{
    counter_all++;
    std::string new_friendly;
    std::string new_compiled;
    ScriptParser parser(yampt::rec_type::SCTX, *merger, friendly_text, compiled_data);
    parser.convertScript();
    new_friendly = parser.getNewFriendly();
    new_compiled = parser.getNewCompiled();
    setToConvertFlag(friendly_text, new_friendly);
    return make_pair(new_friendly, new_compiled);
}

//----------------------------------------------------------
void EsmConverter::setToConvertFlag(const std::string &friendly_text,
                                    const std::string &new_friendly)
{
    if(new_friendly != friendly_text)
    {
        to_convert = true;
        counter_converted++;
    }
    else
    {
        to_convert = false;
        counter_skipped++;
    }
}

//----------------------------------------------------------
std::string EsmConverter::addDialogTopicsToNotConvertedINFOStrings(const std::string &friendly_text)
{
    counter_added++;
    std::string unique_text;
    std::string new_friendly;
    std::string new_friendly_lc;
    size_t pos;

    new_friendly = friendly_text;
    new_friendly_lc = friendly_text;
    transform(new_friendly_lc.begin(), new_friendly_lc.end(),
              new_friendly_lc.begin(), ::tolower);

    for(const auto &elem : merger->getDict(yampt::rec_type::DIAL))
    {
        unique_text = elem.first;
        transform(unique_text.begin(), unique_text.end(),
                  unique_text.begin(), ::tolower);

        if(unique_text != elem.second)
        {
            pos = new_friendly_lc.find(unique_text);
            if(pos != std::string::npos)
            {
                new_friendly.insert(new_friendly.size(), " [" + elem.second + "]");
            }
        }
    }
    return new_friendly;
}

//----------------------------------------------------------
void EsmConverter::convertCELL()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "CELL")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("NAME");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                new_friendly = setNewFriendly(yampt::rec_type::CELL, esm.getUniqueText(), esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(yampt::rec_type::CELL);
}

//----------------------------------------------------------
void EsmConverter::convertPGRD()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "PGRD")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("NAME");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                new_friendly = setNewFriendly(yampt::rec_type::CELL, esm.getUniqueText(), esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(yampt::rec_type::PGRD);
}

//----------------------------------------------------------
void EsmConverter::convertANAM()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("ANAM");
            esm.setFirstFriendlyTo("ANAM");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                new_friendly = setNewFriendly(yampt::rec_type::CELL, esm.getUniqueText(), esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(yampt::rec_type::ANAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCVR()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFirstFriendlyTo("SCVR");
            if(esm.getUniqueStatus() == true)
            {
                while(esm.getFriendlyStatus() == true)
                {
                    if(esm.getFriendlyText().substr(1, 1) == "B")
                    {
                        new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                                      esm.getFriendlyText().substr(5),
                                                      esm.getFriendlyText().substr(5));
                        new_friendly = esm.getFriendlyText().substr(0, 5) + new_friendly;
                        if(to_convert == true)
                        {
                            convertRecordContent(new_friendly);
                        }
                    }
                    esm.setNextFriendlyTo("SCVR");
                }
            }
        }
    }
    printLogLine(yampt::rec_type::SCVR);
}

//----------------------------------------------------------
void EsmConverter::convertDNAM()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "CELL" ||
           esm.getRecordId() == "NPC_")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("DNAM");
            if(esm.getUniqueStatus() == true)
            {
                while(esm.getFriendlyStatus() == true)
                {
                    new_friendly = setNewFriendly(yampt::rec_type::CELL, esm.getFriendlyText(), esm.getFriendlyText());
                    if(to_convert == true)
                    {
                        convertRecordContent(new_friendly + '\0');

                    }
                    esm.setNextFriendlyTo("DNAM");
                }
            }
        }
    }
    printLogLine(yampt::rec_type::DNAM);
}

//----------------------------------------------------------
void EsmConverter::convertCNDT()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "NPC_")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("CNDT");
            if(esm.getUniqueStatus() == true)
            {
                while(esm.getFriendlyStatus() == true)
                {
                    new_friendly = setNewFriendly(yampt::rec_type::CELL, esm.getFriendlyText(), esm.getFriendlyText());
                    if(to_convert == true)
                    {
                        convertRecordContent(new_friendly + '\0');
                    }
                    esm.setNextFriendlyTo("CNDT");
                }
            }
        }
    }
    printLogLine(yampt::rec_type::CNDT);
}

//----------------------------------------------------------
void EsmConverter::convertGMST()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "GMST")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("STRV");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText().substr(0, 1) == "s")
            {
                new_friendly = setNewFriendly(yampt::rec_type::GMST, esm.getUniqueText(), esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::GMST);
}

//----------------------------------------------------------
void EsmConverter::convertFNAM()
{
    std::string unique_text;
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "ACTI" ||
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
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("FNAM");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText() != "player")
            {
                unique_text = esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                new_friendly = setNewFriendly(yampt::rec_type::FNAM, unique_text, esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(yampt::rec_type::FNAM);
}

//----------------------------------------------------------
void EsmConverter::convertDESC()
{
    std::string unique_text;
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "BSGN" ||
           esm.getRecordId() == "CLAS" ||
           esm.getRecordId() == "RACE")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("DESC");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_text = esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                new_friendly = setNewFriendly(yampt::rec_type::DESC, unique_text, esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::DESC);
}

//----------------------------------------------------------
void EsmConverter::convertTEXT()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "BOOK")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("TEXT");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                new_friendly = setNewFriendly(yampt::rec_type::TEXT, esm.getUniqueText(), esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::TEXT);
}

//----------------------------------------------------------
void EsmConverter::convertRNAM()
{
    std::string unique_text;
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "FACT")
        {
            esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("RNAM");
            if(esm.getUniqueStatus() == true)
            {
                while(esm.getFriendlyStatus() == true)
                {
                    unique_text = esm.getUniqueText() + yampt::sep[0] + std::to_string(esm.getFriendlyCounter());
                    new_friendly = setNewFriendly(yampt::rec_type::RNAM, unique_text, esm.getFriendlyText());
                    if(to_convert == true)
                    {
                        new_friendly.resize(32);
                        convertRecordContent(new_friendly);
                    }
                    esm.setNextFriendlyTo("RNAM");
                }
            }
        }
    }
    printLogLine(yampt::rec_type::TEXT);
}

//----------------------------------------------------------
void EsmConverter::convertINDX()
{
    std::string unique_text;
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "SKIL" ||
           esm.getRecordId() == "MGEF")
        {
            esm.setUniqueToINDX();
            esm.setFirstFriendlyTo("DESC");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_text = esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                new_friendly = setNewFriendly(yampt::rec_type::INDX, unique_text, esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::INDX);
}

//----------------------------------------------------------
void EsmConverter::convertDIAL()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "DIAL")
        {
            esm.setUniqueToDialogType();
            esm.setFirstFriendlyTo("NAME");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText() == "T")
            {
                new_friendly = setNewFriendly(yampt::rec_type::DIAL, esm.getFriendlyText(), esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly + '\0');
                }
            }
        }
    }
    printLogLine(yampt::rec_type::DIAL);
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
    std::string unique_text;
    std::string new_friendly;
    std::string dialog_topic;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "DIAL")
        {
            esm.setUniqueToDialogType();
            esm.setFirstFriendlyTo("NAME");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                dialog_topic = esm.getUniqueText() + yampt::sep[0] + esm.getFriendlyText();
            }
        }
        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFirstFriendlyTo("NAME");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_text = dialog_topic + yampt::sep[0] + esm.getUniqueText();
                new_friendly = setNewFriendly(yampt::rec_type::INFO, unique_text, esm.getFriendlyText(), dialog_topic);
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::INFO);
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "INFO")
        {
            esm.setUniqueTo("INAM");
            esm.setFirstFriendlyTo("BNAM");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                new_friendly = setNewScriptBNAM(esm.getFriendlyText());
                if(to_convert == true)
                {
                    convertRecordContent(new_friendly);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::BNAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCPT()
{
    std::string friendly_text;
    std::string compiled_data;
    std::pair<std::string, std::string> new_script;
    std::string new_header;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "SCPT")
        {
            esm.setUniqueTo("SCHD");
            esm.setFirstFriendlyTo("SCDT", false);
            if(esm.getFriendlyStatus() == true)
            {
                compiled_data = esm.getFriendlyText();
            }
            esm.setFirstFriendlyTo("SCTX");
            if(esm.getFriendlyStatus() == true)
            {
                friendly_text = esm.getFriendlyText();
            }
            if(esm.getUniqueStatus() == true)
            {
                new_script = setNewScriptSCPT(friendly_text, compiled_data);
                if(to_convert == true)
                {
                    esm.setFirstFriendlyTo("SCTX");
                    convertRecordContent(new_script.first);
                    esm.setFirstFriendlyTo("SCDT", false);
                    convertRecordContent(new_script.second);

                    // Compiled script data size in script name
                    esm.setFirstFriendlyTo("SCHD", false);
                    new_header = esm.getFriendlyText();
                    new_header.erase(44, 4);
                    new_header.insert(44, tools.convertUIntToStringByteArray(new_script.second.size()));
                    convertRecordContent(new_header);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::SCTX);
}

//----------------------------------------------------------
void EsmConverter::convertGMDT()
{
    std::string friendly_text;
    std::string prefix;
    std::string suffix;
    std::string new_friendly;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "TES3")
        {
            esm.setFirstFriendlyTo("GMDT", false);
            if(esm.getFriendlyStatus() == true)
            {
                friendly_text = esm.getFriendlyText().substr(24, 64);
                friendly_text = tools.eraseNullChars(friendly_text);
                prefix = esm.getFriendlyText().substr(0, 24);
                suffix = esm.getFriendlyText().substr(88);
                new_friendly = setNewFriendly(yampt::rec_type::CELL, friendly_text, friendly_text);
                if(to_convert == true)
                {
                    new_friendly.resize(64);
                    convertRecordContent(prefix + new_friendly + suffix);
                }
            }
        }

        if(esm.getRecordId() == "GAME")
        {
            esm.setFirstFriendlyTo("GMDT", false);
            if(esm.getFriendlyStatus() == true)
            {
                friendly_text = esm.getUniqueText().substr(0, 64);
                friendly_text = tools.eraseNullChars(friendly_text);
                suffix = esm.getFriendlyText().substr(64);
                new_friendly = setNewFriendly(yampt::rec_type::CELL, friendly_text, friendly_text);
                if(to_convert == true)
                {
                    new_friendly.resize(64);
                    convertRecordContent(new_friendly + suffix);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::GMDT);
}