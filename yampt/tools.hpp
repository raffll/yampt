#pragma once

#include "includes.hpp"

class tools_t
{
public:
    enum class rec_type_t
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

        NPC_FLAG,
        Glossary,

        Unknown,
    };

    enum class CreatorMode
    {
        BASE
    };

    enum class Encoding
    {
        UNKNOWN,
        WINDOWS_1250
    };

    struct RecordEntry
    {
        std::string key_text;
        std::string old_text;
        std::string new_text;
        std::string status;
    };

    struct Chapter
    {
        std::vector<RecordEntry> records;
        std::unordered_map<std::string, size_t> index;

        bool insert(const RecordEntry & entry);
        RecordEntry * find(const std::string & id);
        const RecordEntry * find(const std::string & id) const;
        size_t size() const { return records.size(); }
        bool empty() const { return records.empty(); }
    };

    using dict_t = std::map<rec_type_t, Chapter>;

    struct Status
    {
        static constexpr const char * untranslated = "untranslated";
        static constexpr const char * translated = "translated";
        static constexpr const char * auto_identical = "auto_identical";
        static constexpr const char * auto_heuristic = "auto_heuristic";
        static constexpr const char * changed = "changed";
        static constexpr const char * has_errors = "has_errors";
    };

    struct Entry
    {
        const std::string key_text;
        std::string val_text;
        const tools_t::rec_type_t type;
        const std::string optional = "";
    };

    struct Name
    {
        std::string full;
        std::string name;
        std::string ext;

        void setName(const std::string & path)
        {
            full = path.substr(path.find_last_of("\\/") + 1);
            name = full.substr(0, full.find_last_of("."));
            ext = full.substr(full.rfind("."));
        }
    };

    struct Record
    {
        const std::string id;
        std::string content;
        size_t size = 0;
        bool modified = false;
    };

    static const std::vector<std::string> keywords;

    static std::string readFile(const std::string & path);
    static void writeText(
        const std::string & text,
        const std::string & name);
    static void writeFile(
        const std::vector<Record> & records,
        const std::string & name);
    static void createFile(
        const std::vector<Record> & records,
        const std::string & name);
    static size_t getNumberOfElementsInDict(const dict_t & dict);
    static size_t convertStringByteArrayToUInt(const std::string & str);
    static std::string convertUIntToStringByteArray(const size_t size);
    static bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
    static std::string eraseNullChars(std::string str);
    static std::string trimCR(std::string str);
    static std::string replaceNonReadableCharsWithDot(const std::string & str);
    static void addLog(
        const std::string & entry,
        const bool silent = false);
    static std::string getLog() { return log1 + log2; }
    static bool hasError() { return error_flag; }
    static void resetLog();
    static dict_t initializeDict();
    static std::string type2Str(tools_t::rec_type_t type);
    static rec_type_t str2Type(const std::string & str);
    static std::string getDialogType(const std::string & content);
    static std::string getINDX(const std::string & content);
    static bool isFNAM(const std::string & rec_id);

private:
    static std::string log1;
    static std::string log2;
    static bool error_flag;
};
