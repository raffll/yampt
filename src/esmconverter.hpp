#ifndef ESMCONVERTER_HPP
#define ESMCONVERTER_HPP

#include "config.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"
#include "scriptparser.hpp"

class EsmConverter
{
public:
    void convertEsm();
    void writeEsm();

    bool getStatus() { return status; }
    std::string getNameFull() { return esm.getNameFull(); }
    std::string getNamePrefix() { return esm.getNamePrefix(); }
    std::vector<std::string> getRecordColl() { return esm.getRecordColl(); }

    EsmConverter(std::string path, DictMerger &merger, bool add_dial);

private:
    void resetCounters();
    void convertRecordContent(const std::string &new_friendly);
    std::string setNewFriendly(const yampt::rec_type type,
                               const std::string &unique_text,
                               const std::string &friendly_text,
                               const std::string &dialog_topic = "");
    std::string setNewScriptBNAM(const std::string &prefix,
                                 const std::string &friendly_text);
    std::pair<std::string, std::string> setNewScriptSCPT(const std::string &prefix,
                                                         const std::string &friendly_text,
                                                         const std::string &compiled_data);
    std::string addDialogTopicsToNotConvertedINFOStrings(const std::string &friendly_text);
    void setToConvertFlag(const std::string &friendly_text,
                          const std::string &new_friendly);

    void printLogLine(const yampt::rec_type type);

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
    void convertGMDT();

    EsmReader esm;
    DictMerger *merger;
    Tools tools;

    bool status = false;
    const bool add_dial = false;

    int counter_converted = 0;
    int counter_skipped = 0;
    int counter_unchanged = 0;
    int counter_all = 0;
    int counter_added = 0;

    bool to_convert;
};

#endif // ESMCONVERTER_HPP
