#pragma once

#include "includes.hpp"
#include "tools.hpp"
#include "esmreader.hpp"
#include "dictmerger.hpp"

class EsmConverter
{
public:
    const auto & isLoaded() { return esm.isLoaded(); }
    const auto & getName() { return esm.getName(); }
    const auto & getTime() { return esm.getTime(); }
    const auto & getRecords() { return esm.getRecords(); }

    EsmConverter(
        const std::string & path,
        const DictMerger & merger,
        const bool add_hyperlinks,
        const std::string & file_suffix,
        const tools_t::Encoding encoding,
        const bool create_header);

private:
    void convertEsm();
    void resetCounters();
    void convertRecordContent(const std::string & new_text);
    void addNullTerminatorIfEmpty(
        std::string & new_text);
    bool makeNewText(
        const tools_t::Entry & entry,
        std::string & new_text);
    bool isIdentical(
        const std::string & old_text,
        const std::string & new_text);
    void printLogLine(const tools_t::rec_type_t type);
    void convertMAST();
    void createHeader();
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

    tools_t::Encoding detectEncoding();
    bool detectWindows1250Encoding(
        const std::string & text);

    EsmReader esm;
    const DictMerger & merger;

    bool add_hyperlinks;
    const std::string file_suffix;
    const bool create_header;

    int counter_converted = 0;
    int counter_identical = 0;
    int counter_unchanged = 0;
    int counter_all = 0;
    int counter_added = 0;

    tools_t::Encoding esm_encoding = tools_t::Encoding::UNKNOWN;
};
