#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <sstream>
//#include <cctype>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>
//#include <locale>
#include <regex>
#include <ctime>

#include <boost/filesystem/operations.hpp>

#ifdef DEBUG
#define DEBUG_MSG(str) std::cout << str << std::endl;
#else
#define DEBUG_MSG(str)
#endif

namespace yampt
{

struct CaseAwareCompare
{
    bool operator()(const char * left, const char * right) const
    {
        bool tied = true;
        bool tiebreaker = false;
        int i;

        for(i = 0; left[i] != 0; ++i)
        {
            if(right[i] == 0)
            {
                return false;
            }
            if(tolower(left[i]) != tolower(right[i]))
            {
                return tolower(left[i]) < tolower(right[i]);
            }
            if(tied && left[i] != right[i])
            {
                tied = false;
                tiebreaker = left[i] < right[i];
            }
        }
        return(right[i] != 0) || (!tied && tiebreaker);
    }

    bool operator()(const std::string & left, const std::string & right) const
    {
        return operator()(left.c_str(), right.c_str());
    }
};

enum rec_type { CELL, DIAL, INDX, RNAM, DESC, GMST, FNAM, INFO, TEXT, BNAM, SCTX,
                Wilderness, Region, PGRD, ANAM, SCVR, DNAM, CNDT, GMDT };
enum ins_mode { RAW, BASE, ALL, NOTFOUND, CHANGED };
typedef std::array<std::map<std::string, std::string>, 11> dict_t;
typedef std::map<std::string, std::string> inner_dict_t;
const std::vector<std::string> type_name { "CELL", "DIAL", "INDX", "RNAM", "DESC", "GMST", "FNAM", "INFO", "TEXT", "BNAM", "SCTX",
                                           "Wilderness", "Region", "PGRD", "ANAM", "SCVR", "DNAM", "CNDT", "GMDT" };
const std::array<std::string, 5> dialog_type = { "T", "V", "G", "P", "J" };
const std::vector<std::string> sep = { "^", "<_id>", "</_id>", "<key>", "</key>", "<val>", "</val>", "<rec name=\"", "\"/>" };
const std::vector<std::string> err = { "<err name=\"", "\"/>" };
const std::vector<std::string> keyword_list = { "messagebox", "choice", "say ", "say," };

}

class Tools
{
public:
    void writeDict(const yampt::dict_t &dict,
                   const std::string &name);
    void writeText(const std::string &text,
                   const std::string &name);
    void writeFile(const std::vector<std::string> &rec_coll,
                   const std::string &name);
    int getNumberOfElementsInDict(const yampt::dict_t &dict);
    unsigned int convertStringByteArrayToUInt(const std::string &str);
    std::string convertUIntToStringByteArray(const unsigned int x);
    bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
    std::string eraseNullChars(std::string &str);
    std::string eraseCarriageReturnChar(std::string &str);
    std::string replaceNonReadableCharsWithDot(const std::string &str);
    std::string addDialogTopicsToINFOStrings(yampt::inner_dict_t dict,
                                             const std::string &friendly_text,
                                             bool extended);
};

#endif // CONFIG_HPP
