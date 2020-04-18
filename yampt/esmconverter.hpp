#ifndef ESMCONVERTER_HPP
#define ESMCONVERTER_HPP

#include "config.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class EsmConverter
{
public:
    std::string getNameFull() { return esm.getNameFull(); }
    std::string getNamePrefix() { return esm.getNamePrefix(); }
    std::string getNameSuffix() { return esm.getNameSuffix(); }
    std::time_t getTime() { return esm.getTime(); }
    std::vector<std::string> getRecordColl() { return esm.getRecordColl(); }

    EsmConverter(std::string path, DictMerger & merger, bool add_dial, std::string file_suffix, Tools::safe_mode safe_mode);

private:
    void convertEsm(const bool safe);
    void resetCounters();
    void convertRecordContent(const std::string & new_friendly);
    std::string addNullTerminatorIfEmpty(const std::string & new_friendly);
    std::string setNewFriendly(
        const yampt::rec_type type,
        const std::string & unique_text,
        const std::string & friendly_text,
        const std::string & dialog_topic = "");
    std::pair<std::string, std::string> setNewScript(
        const yampt::rec_type type,
        const std::string & prefix,
        const std::string & friendly_text,
        const std::string & compiled_data);
    void checkIfIdentical(
        const std::string & friendly_text,
        const std::string & new_friendly);
    void printLogLine(const yampt::rec_type type);
    void convertMAST();
    void convertCELL();
    void convertPGRD();
    void convertANAM();
    void convertSCVR();
    void convertDNAM();
    void convertCNDT();
    void convertGMST();
    void convertFNAM();
    void convertDESC();
    void convertTEXT();
    void convertRNAM();
    void convertINDX();
    void convertDIAL();
    void convertINFO();
    void convertBNAM();
    void convertSCPT();

    EsmReader esm;
    DictMerger * merger;
    Tools tools;

    const bool add_dial;
    const std::string file_suffix;

    int counter_converted = 0;
    int counter_skipped = 0;
    int counter_unchanged = 0;
    int counter_all = 0;
    int counter_added = 0;

    bool to_convert;
};

#endif // ESMCONVERTER_HPP
