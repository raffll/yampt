#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "includes.hpp"
#include "tools.hpp"

class EsmReader
{
public:
    void selectRecord(size_t i);
    void replaceRecord(const std::string & new_rec);

    void setKey(const std::string & id);
    void setValue(const std::string & id);
    void setNextValue(const std::string & id);

    bool isLoaded() { return is_loaded; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }
    std::time_t getTime() { return time; }
    std::vector<std::string> const & getRecords() const { return records; }

    std::string getRecordContent() { return *rec; }
    std::string getRecordId() { return rec_id; }

    struct SubRecord
    {
        std::string id;
        std::string content;
        std::string text;
        size_t pos = 0;
        size_t size = 0;
        size_t counter = 0;
        bool exist = false;
    };

    const SubRecord & getKey() { return key; }
    const SubRecord & getValue() { return value; }

    Tools::Encoding detectEncoding();

    EsmReader() = default;
    EsmReader(const std::string & path);

private:
    void splitFile(
        const std::string & content,
        const std::string & path);
    void setName(const std::string & path);
    void setTime(const std::string & path);
    void mainLoop(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_id,
        std::string & cur_text,
        EsmReader::SubRecord & subrecord);
    void handleException(const std::exception & e);
    bool detectWindows1250Encoding(const std::string & text);

    std::vector<std::string> records;
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
