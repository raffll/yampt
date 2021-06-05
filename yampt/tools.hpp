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

        PGRD,
        ANAM,
        SCVR,
        DNAM,
        CNDT,
        GMDT,

        Default,
        REGN,

        Glossary,
        NPC_FLAG,

        Annotations,
        Unknown,
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

    struct Entry
    {
        const std::string & key_text;
        const std::string & val_text;
        const Tools::RecType & type;
        const std::string optional;
    };

    struct Name
    {
        std::string full;
        std::string prefix;
        std::string suffix;

        void setName(const std::string & path)
        {
            full = path.substr(path.find_last_of("\\/") + 1);
            prefix = full.substr(0, full.find_last_of("."));
            suffix = full.substr(full.rfind("."));
        }
    };

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
        const Chapter & chapter,
        const std::string & source,
        const bool extended);
    static void addLog(
        const std::string & entry,
        const bool silent = false);
    static std::string getLog() { return log1 + log2; }
    static Dict initializeDict();
    static std::string type2Str(Tools::RecType type);
    static RecType str2Type(const std::string & str);
    static std::string getDialogType(const std::string & content);
    static std::string getINDX(const std::string & content);

private:
    static std::string log1;
    static std::string log2;
};

#endif // TOOLS_HPP
