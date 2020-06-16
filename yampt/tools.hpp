#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "includes.hpp"

class Tools
{
public:
    enum class RecType
    {
        CELL,
        DIAL,
        INDX,
        RNAM,
        DESC,
        GMST,
        FNAM,
        INFO,
        TEXT,
        BNAM,
        SCTX,

        Glossary,
        Annotation,

        Wilderness,
        Region,
        PGRD,
        ANAM,
        SCVR,
        DNAM, 
        CNDT,
        GMDT
    };

    enum class CreatorMode
    {
        RAW,
        BASE,
        ALL,
        NOTFOUND,
        CHANGED
    };

    enum class Encoding
    {
        UNKNOWN,
        WINDOWS_1250
    };

    using Chapter = std::map<std::string, std::string>;
    using Dict = std::map<RecType, Chapter>;


    static const std::vector<std::string> dialog_type;
    static const std::vector<std::string> sep;
    static const std::vector<std::string> err;
    static const std::vector<std::string> keywords;

    static std::string readFile(const std::string & path);
    static void writeDict(
        const Dict & dict,
        const std::string & name);
    static void writeText(
        const std::string & text,
        const std::string & name);
    static void writeFile(
        const std::vector<std::string> & rec_coll,
        const std::string & name);
    static size_t getNumberOfElementsInDict(const Dict & dict);
    static size_t convertStringByteArrayToUInt(const std::string & str);
    static std::string convertUIntToStringByteArray(const size_t size);
    static bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
    static std::string eraseNullChars(std::string str);
    static std::string trimCR(std::string str);
    static std::string replaceNonReadableCharsWithDot(const std::string & str);
    static std::string addHyperlinks(
        Chapter dict,
        const std::string & val_text,
        bool extended);
    static void addLog(
        const std::string & entry,
        const bool silent = false);
    static std::string getLog() { return log; }
    static Dict initializeDict();
    static std::string getTypeName(Tools::RecType type);

private:
    static std::string log;
};

#endif // TOOLS_HPP
