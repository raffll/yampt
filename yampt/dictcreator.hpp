#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class DictCreator
{
public:
    std::string getNameFull() { return esm_n.getNameFull(); }
    std::string getNamePrefix() { return esm_n.getNamePrefix(); }
    Tools::Dict const & getDict() const { return dict; }

    DictCreator(const std::string & path_n);
    DictCreator(
        const std::string & path_n,
        const std::string & path_f);
    DictCreator(
        const std::string & path_n,
        const DictMerger & merger,
        const Tools::CreatorMode mode,
        const bool add_dial);

private:
    void makeDict(const bool same_order);
    bool isSameOrder();
    void resetCounters();
    std::string translateDialogTopicsInDictId(std::string to_translate);
    void validateRecord();
    void validateRecordForModeALL();
    void validateRecordForModeNOT();
    void validateRecordForModeCHANGED();
    void insertRecordToDict();
    std::vector<std::string> makeScriptMessages(const std::string & new_friendly);

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

    EsmReader esm_n;
    EsmReader esm_f;
    EsmReader * esm_ptr;
    const DictMerger * merger;
    Tools::Dict dict;

    std::string key_text;
    std::string val_text;
    Tools::RecType type;

    std::vector<std::string> * message_ptr;
    std::vector<std::string> message_n;
    std::vector<std::string> message_f;

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

    std::vector<Pattern> patterns_f;
    std::map<std::string, size_t> patterns_n;
};

#endif // DICTCREATOR_HPP
