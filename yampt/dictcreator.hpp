#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class DictCreator
{
public:
    const auto & getName() { return esm.getName(); }
    const auto & getDict() const { return dict; }

    DictCreator(
        const std::string & path);
    DictCreator(
        const std::string & path,
        const std::string & path_ext);
    DictCreator(
        const std::string & path,
        const DictMerger & merger,
        const Tools::CreatorMode mode);

private:
    struct Pattern
    {
        std::string str;
        size_t pos;
        bool missing;
    };

    using PatternsExt = std::vector<Pattern>;
    using Patterns = std::map<std::string, size_t>;

    void makeDict(const bool same_order);
    bool isSameOrder();
    void resetCounters();
    std::string translateDialogTopic(std::string to_translate);
    void validateEntry(const Tools::Entry & entry);
    void validateRecordForModeALL(const Tools::Entry & entry);
    void validateRecordForModeNOT(const Tools::Entry & entry);
    void validateRecordForModeCHANGED(const Tools::Entry & entry);
    void makeAnnotations(const Tools::Entry & entry);
    void insertRecordToDict(const Tools::Entry & entry);
    std::vector<std::string> makeScriptMessages(const std::string & script_text);
    void printLogLine(const Tools::RecType type);

    void makeDictCELL();
    void makeDictCELL_Default();
    void makeDictCELL_Unordered_Default();
    void makeDictCELL_REGN();
    void makeDictCELL_Unordered_REGN();
    void makeDictGMST();
    void makeDictFNAM();
    void makeDictDESC();
    void makeDictTEXT();
    void makeDictRNAM();
    void makeDictINDX();
    void makeDictDIAL();
    void makeDictNPC_FLAG();
    void makeDictINFO();
    void makeDictBNAM();
    void makeDictSCPT();
    void makeDictFNAM_Glossary();
    bool isFNAM(const std::string & rec_id);

    void makeDictCELL_Unordered();
    PatternsExt makeDictCELL_Unordered_PatternsExt();
    Patterns makeDictCELL_Unordered_Patterns();
    std::string makeDictCELL_Unordered_Pattern(EsmReader & esm_cur);
    void makeDictCELL_Unordered_AddMissing(const PatternsExt & patterns_ext);

    void makeDictDIAL_Unordered();
    PatternsExt makeDictDIAL_Unordered_PatternsExt();
    Patterns makeDictDIAL_Unordered_Patterns();
    std::string makeDictDIAL_Unordered_Pattern(EsmReader & esm_cur, size_t i);
    void makeDictDIAL_Unordered_AddMissing(const PatternsExt & patterns_ext);

    void makeDictBNAM_Unordered();
    PatternsExt makeDict_Unordered_PatternsExt(
        const std::string & rec_id,
        const std::string & key_id);
    Patterns makeDictBNAM_Unordered_Patterns();

    void makeDictSCPT_Unordered();
    Patterns makeDictSCPT_Unordered_Patterns();

    EsmReader esm;
    EsmReader esm_ext;
    EsmReader & esm_ref;
    const DictMerger & merger;
    const Tools::CreatorMode mode;
    Tools::Dict dict;

    int counter_created = 0;
    int counter_missing = 0;
    int counter_doubled = 0;
    int counter_identical = 0;
    int counter_all = 0;
};

#endif // DICTCREATOR_HPP
