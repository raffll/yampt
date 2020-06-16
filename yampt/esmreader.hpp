#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "includes.hpp"
#include "tools.hpp"

class EsmReader
{
public:
    void setRecord(size_t i);
    void replaceRecordContent(const std::string & new_rec);

    void setKey(const std::string & id);
    void setValue(const std::string & id);
    void setNextValue(const std::string & id);

    bool isLoaded() { return is_loaded; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }
    std::time_t getTime() { return time; }
    std::vector<std::string> const & getRecords() const { return rec_coll; }

    std::string getRecordContent() { return *rec; }
    std::string getRecordId() { return rec_id; }

    std::string getUniqueWithNull() { return key.text; }
    std::string getUniqueText() { return Tools::eraseNullChars(key.text); }
    std::string getUniqueId() { return key.id; }
    bool isUniqueValid() { return key.exist; }

    std::string getFriendlyWithNull() { return value.text; }
    std::string getFriendlyText() { return Tools::eraseNullChars(value.text); }
    std::string getFriendlyId() { return value.id; }
    size_t getFriendlyPos() { return value.pos; }
    size_t getFriendlySize() { return value.size; }
    std::string getFriendlyCounter() { return std::to_string(value.counter); }
    bool isFriendlyValid() { return value.exist; }

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

    bool detectWindows1250Encoding(const std::string & val_text);

    std::vector<std::string> rec_coll;
    std::string name_full;
    std::string name_prefix;
    std::string name_suffix;
    std::time_t time = 0;
    bool is_loaded = false;

    std::string * rec = nullptr;
    size_t rec_size = 0;
    std::string rec_id;

    SubRecord key;
    SubRecord value;
};

#endif // ESMREADER_HPP
