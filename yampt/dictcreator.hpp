#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "config.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class DictCreator
{
public:
    std::string getNameFull() { return esm_n.getNameFull(); }
    std::string getNamePrefix() { return esm_n.getNamePrefix(); }

    Tools::dict_t const & getDict() const { return dict; }
    Tools::single_dict_t const & getDict(Tools::RecType type) const { return dict[type]; }
    Tools::single_dict_t const & getDict(size_t type) const { return dict[type]; }

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

    void validateRecord(
        const std::string & unique_text,
        const std::string & friendly_text,
        const Tools::RecType type);
    void validateRecordForModeALL(
        const std::string & unique_text,
        const std::string & friendly_text,
        const Tools::RecType type);
    void validateRecordForModeNOT(
        const std::string & unique_text,
        const std::string & friendly_text,
        const Tools::RecType type);
    void validateRecordForModeCHANGED(
        const std::string & unique_text,
        const std::string & friendly_text,
        const Tools::RecType type);

    void insertRecordToDict(
        const std::string & unique_text,
        const std::string & friendly_text,
        const Tools::RecType type);
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
    Tools::dict_t dict;

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

    struct pattern
    {
        std::string str;
        size_t pos;
        bool missing;
    };

    std::vector<pattern> patterns_f;
    std::map<std::string, size_t> patterns_n;
};

#endif // DICTCREATOR_HPP
