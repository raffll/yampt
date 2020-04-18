#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <regex>
#include <ctime>

#include <boost/filesystem/operations.hpp>

#ifdef DEBUG
#define DEBUG_MSG(str) std::cout << str << std::endl;
#else
#define DEBUG_MSG(str)
#endif

class Tools
{
public:
    enum rec_type
    {
        CELL, DIAL, INDX, RNAM, DESC,
        GMST, FNAM, INFO, TEXT, BNAM,
        SCTX, Wilderness, Region, PGRD,
        ANAM, SCVR, DNAM, CNDT, GMDT
    };

    enum ins_mode
    {
        RAW,
        BASE,
        ALL,
        NOTFOUND,
        CHANGED
    };

    enum class safe_mode
    {
        heuristic,
        enabled,
        disabled
    };

    using dict_t = std::array<std::map<std::string, std::string>, 11>;
    using single_dict_t = std::map<std::string, std::string>;

    static const std::vector<std::string> type_name;
    static const std::array<std::string, 5> dialog_type;
    static const std::vector<std::string> sep;
    static const std::vector<std::string> err;
    static const std::vector<std::string> keyword_list;

    static std::string readFile(const std::string & path);
    static void writeDict(
        const dict_t & dict,
        const std::string & name);
    static void writeText(
        const std::string & text,
        const std::string & name);
    static void writeFile(
        const std::vector<std::string> & rec_coll,
        const std::string & name);
    static int getNumberOfElementsInDict(const dict_t & dict);
    static unsigned int convertStringByteArrayToUInt(const std::string & str);
    static std::string convertUIntToStringByteArray(const unsigned int x);
    static bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
    static std::string eraseNullChars(std::string str);
    static std::string eraseCarriageReturnChar(std::string str);
    static std::string replaceNonReadableCharsWithDot(const std::string & str);
    static std::string addDialogTopicsToINFOStrings(
        single_dict_t dict,
        const std::string & friendly_text,
        bool extended);
    void addLog(const std::string log);
    void clearLog();
    std::string getLog() { return log; }

private:
    std::string log;
};

#endif // CONFIG_HPP
