#ifndef DICTCREATOR_HPP
#define DICTCREATOR_HPP

#include "config.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class DictCreator
{
public:
    void makeDict();

    bool getStatus() { return status; }
    std::string getNameFull() { return esm_n.getNameFull(); }
    std::string getNamePrefix() { return esm_n.getNamePrefix(); }
    yampt::dict_t const& getDict() const { return dict; }

    DictCreator(const std::string &path_n);
    DictCreator(const std::string &path_n,
                const std::string &path_f);
    DictCreator(const std::string &path_n,
                DictMerger &merger,
                const yampt::ins_mode mode);

private:
    void makeDictBasic();
    void makeDictExtended();
    bool compareMasterFiles();
    void resetCounters();
    std::string dialTranslator(std::string to_translate);
    void validateRecord(const std::string &unique_text,
                        const std::string &friendly_text,
                        const yampt::rec_type type);
    void insertRecord(const std::string &unique_text,
                      const std::string &friendly_text,
                      const yampt::rec_type type);
    std::vector<std::string> makeMessageColl(const std::string &new_friendly);

    void printLog(const yampt::rec_type type);
    void printLogHeader();

    void makeDictCELL();
    void makeDictCELLExtended();
    std::vector<std::tuple<std::string, size_t, bool> > makeDictCELLExtendedPattern();
    std::map<std::string, size_t> makeDictCELLExtendedMatch();
    void makeDictCELLWilderness();
    void makeDictCELLExtendedWilderness();
    void makeDictCELLRegion();
    void makeDictCELLExtendedRegion();
    void makeDictGMST();
    void makeDictFNAM();
    void makeDictDESC();
    void makeDictTEXT();
    void makeDictRNAM();
    void makeDictINDX();
    void makeDictDIAL();
    void makeDictDIALExtended();
    std::vector<std::tuple<std::string, size_t, bool> > makeDictDIALExtendedPattern();
    std::map<std::string, size_t> makeDictDIALExtendedMatch();
    void makeDictINFO();
    void makeDictBNAM();
    void makeDictBNAMExtended();
    std::vector<std::pair<std::string, size_t> > makeDictBNAMExtendedPattern();
    std::map<std::string, size_t> makeDictBNAMExtendedMatch();
    void makeDictSCPT();
    void makeDictSCPTExtended();
    std::vector<std::pair<std::string, size_t> > makeDictSCPTExtendedPattern();
    std::map<std::string, size_t> makeDictSCPTExtendedMatch();

    EsmReader esm_n;
    EsmReader esm_f;
    EsmReader *esm_ptr;
    DictMerger *merger;
    Tools tools;
    yampt::dict_t dict;

    std::vector<std::string> *message_ptr;
    std::vector<std::string> message_n;
    std::vector<std::string> message_f;

    bool status = false;
    bool basic_mode = false;
    const yampt::ins_mode mode;

    int counter_created;
    int counter_missing;
    int counter_identical;
    int counter_all;
};

#endif // DICTCREATOR_HPP
