#pragma once

#include "includes.hpp"
#include "tools.hpp"
#include "esmreader.hpp"

class DictCreator
{
public:
    const auto & getName() { return esm.getName(); }
    const auto & getDict() const { return dict; }

    DictCreator(
        const std::string & plugin_path,
        const Tools::Dict * base_dict = nullptr);

    DictCreator(
        const std::string & path,
        const std::string & path_ext);

private:
    struct Pattern
    {
        std::string str;
        size_t pos;
        bool missing;
    };

    using PatternsExt = std::vector<Pattern>;
    using Patterns = std::map<std::string, size_t>;

    struct IDs
    {
        const std::string & rec_id;
        const std::string & key_id;
        const std::string & val_id;
        const Tools::RecType type;
    };

    void makeDictForMake();
    void makeDictForBase(const bool same_order);
    bool isSameOrder();
    void resetCounters();
    std::string translateDialogTopic(std::string to_translate);
    void insertRecord(const std::string & id, const std::string & original, Tools::RecType type);
    void insertRecordToDict(const std::string & id, const std::string & text, Tools::RecType type);
    std::vector<std::string> makeScriptMessages(const std::string & script_text);
    void printLogLine(const Tools::RecType type);

    void makeDictCELL();
    void makeDictCELL_Default();
    void makeDictCELL_REGN();
    void makeDictGMST();
    void makeDictFNAM();
    void makeDictDESC();
    void makeDictTEXT();
    void makeDictRNAM();
    void makeDictINDX();
    void makeDictDIAL();
    void makeDictNPC_FLAG();
    void makeDictINFO();
    void makeDictScript(const IDs & ids);
    void makeDictFNAM_Glossary();

    void makeDictCELL_Unordered();
    void makeDictCELL_Unordered_Default();
    void makeDictCELL_Unordered_REGN();
    PatternsExt makeDictCELL_Unordered_PatternsExt();
    Patterns makeDictCELL_Unordered_Patterns();
    std::string makeDictCELL_Unordered_Pattern(EsmReader & esm_cur);
    void makeDictCELL_Unordered_AddMissing(const PatternsExt & patterns_ext);

    void makeDictDIAL_Unordered();
    PatternsExt makeDictDIAL_Unordered_PatternsExt();
    Patterns makeDictDIAL_Unordered_Patterns();
    std::string makeDictDIAL_Unordered_Pattern(EsmReader & esm_cur, size_t i);
    void makeDictDIAL_Unordered_AddMissing(const PatternsExt & patterns_ext);

    void makeDictScript_Unordered(const IDs & ids);
    PatternsExt makeDict_Unordered_PatternsExt(const IDs & ids);
    Patterns makeDict_Unordered_Patterns(const IDs & ids);

    EsmReader esm;
    EsmReader esm_ext;
    EsmReader & esm_ref;
    const Tools::Dict * base_dict = nullptr;
    Tools::Dict dict;
    bool is_make_mode = false;

    int counter_created = 0;
    int counter_missing = 0;
    int counter_doubled = 0;
    int counter_identical = 0;
    int counter_all = 0;
};
