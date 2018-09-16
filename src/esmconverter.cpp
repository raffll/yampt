#include "esmconverter.hpp"

//----------------------------------------------------------
EsmConverter::EsmConverter(std::string path,
                           DictMerger &merger,
                           bool add_dial,
                           std::string suffix)
    : esm(path),
      merger(&merger),
      add_dial(add_dial),
      suffix(suffix)
{
    if(esm.isLoaded() == true &&
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
        convertMAST();
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
        //convertGMDT();
    }
}

//----------------------------------------------------------
void EsmConverter::printLogLine(const yampt::rec_type type)
{
    /*if(type == yampt::rec_type::INFO &&
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
    {*/
    std::cout << yampt::type_name[type] << " "
              << std::setw(10) << std::to_string(counter_converted) << " / "
              << std::setw(7) << std::to_string(counter_skipped) << " / "
              << std::setw(9) << std::to_string(counter_unchanged) << " / "
              << std::setw(6) << std::to_string(counter_all) << std::endl;
    //}
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
    size_t rec_size;
    std::string rec_content = esm.getRecordContent();
    rec_content.erase(esm.getFriendlyPos() + 8, esm.getFriendlySize());
    rec_content.insert(esm.getFriendlyPos() + 8, new_friendly);
    rec_content.erase(esm.getFriendlyPos() + 4, 4);
    rec_content.insert(esm.getFriendlyPos() + 4,
                       tools.convertUIntToStringByteArray(new_friendly.size()));
    rec_size = rec_content.size() - 16;
    rec_content.erase(4, 4);
    rec_content.insert(4, tools.convertUIntToStringByteArray(rec_size));
    esm.replaceRecordContent(rec_content);
}

//----------------------------------------------------------
std::string EsmConverter::addNullTerminatorIfEmpty(const std::string &new_friendly)
{
    std::string result;
    if(new_friendly == "")
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
std::string EsmConverter::setNewFriendly(const yampt::rec_type type,
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
        new_friendly = tools.addDialogTopicsToINFOStrings(merger->getDict(yampt::rec_type::DIAL), friendly_text, false);
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
std::string EsmConverter::setNewScriptBNAM(const std::string &prefix,
                                           const std::string &friendly_text)
{
    counter_all++;
    std::string new_friendly;
    ScriptParser parser(yampt::rec_type::BNAM,
                        *merger,
                        prefix,
                        friendly_text);
    parser.convertScript();
    new_friendly = parser.getNewFriendly();
    setToConvertFlag(friendly_text, new_friendly);
    return new_friendly;
}

//----------------------------------------------------------
std::pair<std::string, std::string> EsmConverter::setNewScriptSCPT(const std::string &prefix,
                                                                   const std::string &friendly_text,
                                                                   const std::string &compiled_data)
{
    counter_all++;
    std::string new_friendly;
    std::string new_compiled;
    ScriptParser parser(yampt::rec_type::SCTX,
                        *merger,
                        prefix,
                        friendly_text,
                        compiled_data);
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
void EsmConverter::convertMAST()
{
    std::string master_prefix;
    std::string master_suffix;

    resetCounters();
    for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
    {
        esm.setRecordTo(i);
        if(esm.getRecordId() == "TES3")
        {
            esm.setFirstFriendlyTo("MAST");
            while(esm.getFriendlyStatus() == true)
            {
                master_prefix = esm.getFriendlyText().substr(0, esm.getFriendlyText().find_last_of("."));
                master_suffix = esm.getFriendlyText().substr(esm.getFriendlyText().rfind("."));
                convertRecordContent(master_prefix + suffix + master_suffix + '\0');
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
                new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                              esm.getUniqueText(),
                                              esm.getFriendlyText());
                if(to_convert == true)
                {
                    // Null terminated
                    // Can't be empty
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
                new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                              esm.getUniqueText(),
                                              esm.getFriendlyText());
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
                new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                              esm.getUniqueText(),
                                              esm.getFriendlyText());
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
                            // Not null terminated
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
            //esm.setUniqueTo("NAME");
            esm.setFirstFriendlyTo("DNAM");
            //if(esm.getUniqueStatus() == true)
            //{
                while(esm.getFriendlyStatus() == true)
                {
                    new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                                  esm.getFriendlyText(),
                                                  esm.getFriendlyText());
                    if(to_convert == true)
                    {
                        convertRecordContent(new_friendly + '\0');
                    }
                    esm.setNextFriendlyTo("DNAM");
                }
            //}
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
                    new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                                  esm.getFriendlyText(),
                                                  esm.getFriendlyText());
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
                new_friendly = setNewFriendly(yampt::rec_type::GMST,
                                              esm.getUniqueText(),
                                              esm.getFriendlyText());
                if(to_convert == true)
                {
                    // Null terminated only if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
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
        if(esm.getRecordId() == "ACTI" || esm.getRecordId() == "ALCH" ||
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
            esm.setFirstFriendlyTo("FNAM");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText() != "player")
            {
                unique_text = esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                new_friendly = setNewFriendly(yampt::rec_type::FNAM,
                                              unique_text,
                                              esm.getFriendlyText());
                if(to_convert == true)
                {
                    // Null terminated
                    // Don't exist if empty
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
                new_friendly = setNewFriendly(yampt::rec_type::DESC,
                                              unique_text,
                                              esm.getFriendlyText());
                if(to_convert == true)
                {
                    if(esm.getRecordId() == "BSGN")
                    {
                        // Null terminated
                        // Don't exist if empty
                        convertRecordContent(new_friendly + '\0');
                    }
                    if(esm.getRecordId() == "CLAS" ||
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
                new_friendly = setNewFriendly(yampt::rec_type::TEXT,
                                              esm.getUniqueText(),
                                              esm.getFriendlyText());
                if(to_convert == true)
                {
                    // Not null terminated
                    // Don't exist if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
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
                    new_friendly = setNewFriendly(yampt::rec_type::RNAM,
                                                  unique_text,
                                                  esm.getFriendlyText());
                    if(to_convert == true)
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
    printLogLine(yampt::rec_type::RNAM);
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
            esm.setUniqueTo("INDX");
            esm.setFirstFriendlyTo("DESC");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true)
            {
                unique_text = esm.getRecordId() + yampt::sep[0] + esm.getUniqueText();
                new_friendly = setNewFriendly(yampt::rec_type::INDX,
                                              unique_text,
                                              esm.getFriendlyText());
                if(to_convert == true)
                {
                    // Not null terminated
                    // Don't exist if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
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
            esm.setUniqueTo("DATA");
            esm.setFirstFriendlyTo("NAME");
            if(esm.getUniqueStatus() == true &&
               esm.getFriendlyStatus() == true &&
               esm.getUniqueText() == "T")
            {
                new_friendly = setNewFriendly(yampt::rec_type::DIAL,
                                              esm.getFriendlyText(),
                                              esm.getFriendlyText());
                if(to_convert == true)
                {
                    // Null terminated
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
            esm.setUniqueTo("DATA");
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
                new_friendly = setNewFriendly(yampt::rec_type::INFO,
                                              unique_text,
                                              esm.getFriendlyText(),
                                              dialog_topic);
                if(to_convert == true)
                {
                    // Not null terminated
                    // Don't exist if empty
                    convertRecordContent(addNullTerminatorIfEmpty(new_friendly));
                }
            }
        }
    }
    printLogLine(yampt::rec_type::INFO);
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
    std::string prefix;
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
                prefix = esm.getUniqueText() + yampt::sep[0];
                new_friendly = setNewScriptBNAM(prefix, esm.getFriendlyText());
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
    std::string prefix;
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
                prefix = esm.getUniqueText() + yampt::sep[0];
                new_script = setNewScriptSCPT(prefix, friendly_text, compiled_data);
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
/*void EsmConverter::convertGMDT()
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
                new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                              friendly_text,
                                              friendly_text);
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
                new_friendly = setNewFriendly(yampt::rec_type::CELL,
                                              friendly_text,
                                              friendly_text);
                if(to_convert == true)
                {
                    new_friendly.resize(64);
                    convertRecordContent(new_friendly + suffix);
                }
            }
        }
    }
    printLogLine(yampt::rec_type::GMDT);
}*/
