#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "includes.hpp"
#include "tools.hpp"

class EsmReader
{
public:
    void setRecordTo(size_t i);
    void replaceRecordContent(const std::string & new_rec);

    void setUniqueTo(const std::string & id);
    void setFriendlyTo(const std::string & id);
    void setNextFriendlyTo(const std::string & id);

    bool isLoaded() { return is_loaded; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }
    std::time_t getTime() { return time; }
    std::vector<std::string> const & getRecords() const { return rec_coll; }

    std::string getRecordContent() { return *rec; }
    std::string getRecordId() { return rec_id; }

    std::string getUniqueWithNull() { return unique.text; }
    std::string getUniqueText() { return Tools::eraseNullChars(unique.text); }
    std::string getUniqueId() { return unique.id; }
    bool isUniqueValid() { return unique.exist; }

    std::string getFriendlyWithNull() { return friendly.text; }
    std::string getFriendlyText() { return Tools::eraseNullChars(friendly.text); }
    std::string getFriendlyId() { return friendly.id; }
    size_t getFriendlyPos() { return friendly.pos; }
    size_t getFriendlySize() { return friendly.size; }
    std::string getFriendlyCounter() { return std::to_string(friendly.counter); }
    bool isFriendlyValid() { return friendly.exist; }

    Tools::Encoding detectEncoding();

    EsmReader() = default;
    EsmReader(const std::string & path);

private:
    struct SubRecord
    {
        std::string id;
        std::string text;
        size_t pos = 0;
        size_t size = 0;
        size_t counter = 0;
        bool exist = false;
    };

    void splitFileIntoRecordColl(
        const std::string & content,
        const std::string & path);
    void setName(const std::string & path);
    void setTime(const std::string & path);

    void uniqueMainLoop(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_id,
        std::string & cur_text);
    void caseForDialogType(
        std::size_t & cur_pos,
        std::string & cur_text);
    void caseForINDX(
        std::size_t & cur_pos,
        std::string & cur_text);
    void caseForDefault(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_text);

    void friendlyMainLoop(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_id,
        std::string & cur_text);

    void ifEndOfRecordReached(
        std::size_t & cur_pos,
        EsmReader::SubRecord & subrecord);

    void handleException(const std::exception & e);

    bool detectWindows1250Encoding(const std::string & friendly_text);

    std::vector<std::string> rec_coll;
    std::string name_full;
    std::string name_prefix;
    std::string name_suffix;
    std::time_t time = 0;
    bool is_loaded = false;

    std::string * rec = nullptr;
    size_t rec_size = 0;
    std::string rec_id;

    SubRecord unique;
    SubRecord friendly;
};

#endif // ESMREADER_HPP
