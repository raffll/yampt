#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class DictCreator
{
public:
    std::string getNameFull() { return esm.getNameFull(); }
    std::string getNamePrefix() { return esm.getNamePrefix(); }
    Tools::Dict const & getDict() const { return dict; }

    DictCreator(const std::string & path_n);
    DictCreator(
        const std::string & path_n,
        const std::string & path_f);
    DictCreator(
        const std::string & path_n,
        const DictMerger & merger,
        const Tools::CreatorMode mode,
        const bool add_hyperlinks);

private:
    void makeDict(const bool same_order);
    bool isSameOrder();
    void resetCounters();
    std::string translateDialogTopic(std::string to_translate);
    void validateRecord();
    void validateRecordForModeALL();
    void validateRecordForModeNOT();
    void validateRecordForModeCHANGED();
    void addAnnotations();
    void insertRecordToDict();
    std::vector<std::string> makeScriptMessages(const std::string & script_text);
    void addGenderAnnotations();
    void printLogLine(const Tools::RecType type);

    void makeDictCELL();
    void makeDictCELLWilderness();
    void makeDictCELLWildernessExtended();
    void makeDictCELLRegion();
    void makeDictCELLRegionExtended();
    void makeDictGMST();
    void makeDictFNAM();
    void makeDictDESC();
    void makeDictTEXT();
    void makeDictRNAM();
    void makeDictINDX();
    void makeDictDIAL();
    void makeDictINFO();
    void makeDictBNAM();
    void makeDictSCPT();

    void makeDictCELLExtended();
    void makeDictCELLExtendedForeignColl();
    void makeDictCELLExtendedNativeColl();
    std::string makeDictCELLExtendedPattern(EsmReader & esm_cur);
    void makeDictCELLExtendedAddMissing();

    void makeDictDIALExtended();
    void makeDictDIALExtendedForeignColl();
    void makeDictDIALExtendedNativeColl();
    std::string makeDictDIALExtendedPattern(EsmReader & esm_cur, size_t i);
    void makeDictDIALExtendedAddMissing();

    void makeDictBNAMExtended();
    void makeDictBNAMExtendedForeignColl();
    void makeDictBNAMExtendedNativeColl();

    void makeDictSCPTExtended();
    void makeDictSCPTExtendedForeignColl();
    void makeDictSCPTExtendedNativeColl();

    EsmReader esm;
    EsmReader esm_ext;
    EsmReader * esm_ptr;
    const DictMerger * merger;
    Tools::Dict dict;

    std::string key_text;
    std::string val_text;
    Tools::RecType type;

    std::vector<std::string> message;
    std::vector<std::string> message_ext;
    std::vector<std::string> * message_ptr;

    const Tools::CreatorMode mode;
    const bool add_hyperlinks;

    int counter_created = 0;
    int counter_missing = 0;
    int counter_doubled = 0;
    int counter_identical = 0;
    int counter_all = 0;

    struct Pattern
    {
        std::string str;
        size_t pos;
        bool missing;
    };

    std::vector<Pattern> patterns_ext;
    std::map<std::string, size_t> patterns;
};

#endif // DICTCREATOR_HPP
