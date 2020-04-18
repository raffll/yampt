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
    DictCreator(const std::string & path_n,
                const std::string & path_f);
    DictCreator(const std::string & path_n,
                const DictMerger & merger,
                const Tools::CreatorMode mode,
                const bool add_dial);

private:
    void makeDict(const bool same_order);
    bool isSameOrder();
    void resetCounters();
    std::string translateDialogTopicsInDictId(std::string to_translate);

    void validateRecord(const std::string & unique_text,
                        const std::string & friendly_text,
                        const Tools::RecType type);
    void validateRecordForModeALL(const std::string & unique_text,
                                  const std::string & friendly_text,
                                  const Tools::RecType type);
    void validateRecordForModeNOT(const std::string & unique_text,
                                  const std::string & friendly_text,
                                  const Tools::RecType type);
    void validateRecordForModeCHANGED(const std::string & unique_text,
                                      const std::string & friendly_text,
                                      const Tools::RecType type);

    void insertRecordToDict(const std::string & unique_text,
                            const std::string & friendly_text,
                            const Tools::RecType type);
    std::vector<std::string> makeScriptMessagesColl(const std::string & new_friendly);

    void printLogHeader();
    void printLogSummary();
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
    Tools tools;
    Tools::dict_t dict;

    std::vector<std::string> * message_ptr;
    std::vector<std::string> message_n;
    std::vector<std::string> message_f;

    const Tools::CreatorMode mode;
    const bool add_hyperlinks;

    int counter_created;
    int counter_missing;
    int counter_doubled;
    int counter_identical;
    int counter_all;

    std::vector<std::tuple<std::string, size_t, bool>> foreign_coll;
    std::vector<std::pair<std::string, size_t>> foreign_coll_script;
    std::map<std::string, size_t> native_coll;
};

#endif // DICTCREATOR_HPP
