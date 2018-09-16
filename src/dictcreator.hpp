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

    yampt::dict_t const& getDict() const { return dict; }
    yampt::inner_dict_t const& getDict(yampt::rec_type type) const { return dict[type]; }
    yampt::inner_dict_t const& getDict(size_t type) const { return dict[type]; }

    DictCreator(const std::string &path_n);
    DictCreator(const std::string &path_n,
                const std::string &path_f);
    DictCreator(const std::string &path_n,
                DictMerger &merger,
                const yampt::ins_mode mode);

private:
    void makeDictBasic();
    void makeDictExtended();
    bool areMastersHaveRecordsInSameOrder();
    void resetCounters();
    std::string translateDialogTopicsInDictId(std::string to_translate);

    void validateRecord(const std::string &unique_text,
                        const std::string &friendly_text,
                        const yampt::rec_type type);
    void validateRecordForModeALL(const std::string &unique_text,
                                  const std::string &friendly_text,
                                  const yampt::rec_type type);
    void validateRecordForModeNOT(const std::string &unique_text,
                                  const std::string &friendly_text,
                                  const yampt::rec_type type);
    void validateRecordForModeCHANGED(const std::string &unique_text,
                                      const std::string &friendly_text,
                                      const yampt::rec_type type);

    void insertRecordToDict(const std::string &unique_text,
                            const std::string &friendly_text,
                            const yampt::rec_type type);
    std::vector<std::string> makeScriptMessagesColl(const std::string &new_friendly);

    void printLogLine(const yampt::rec_type type);

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
    std::vector<std::tuple<std::string, size_t, bool> > makeDictCELLExtendedPattern();
    std::map<std::string, size_t> makeDictCELLExtendedMatch();
    void makeDictDIALExtended();
    std::vector<std::tuple<std::string, size_t, bool> > makeDictDIALExtendedPattern();
    std::map<std::string, size_t> makeDictDIALExtendedMatch();
    void makeDictBNAMExtended();
    std::vector<std::pair<std::string, size_t> > makeDictBNAMExtendedPattern();
    std::map<std::string, size_t> makeDictBNAMExtendedMatch();
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

    bool status;
    bool basic_mode;
    const yampt::ins_mode mode;

    int counter_created;
    int counter_missing;
    int counter_doubled;
    int counter_identical;
    int counter_all;
};

#endif // DICTCREATOR_HPP
