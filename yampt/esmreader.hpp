#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "includes.hpp"
#include "tools.hpp"

class EsmReader
{
public:
    void selectRecord(size_t i);
    void replaceRecord(const std::string & content);
    void setModified(size_t i);

    void setKey(const std::string & id);
    void setValue(const std::string & id);
    void setNextValue(const std::string & id);

    const auto & isLoaded() { return is_loaded; }
    const auto & getName() { return name; }
    const auto & getTime() { return time; }
    const auto & getRecords() const { return records; }

    const auto & getRecord() { return *rec; }
    size_t getModifiedCount();

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

    const auto & getKey() { return key; }
    const auto & getValue() { return value; }

    EsmReader() = default;
    EsmReader(const std::string & path);

private:
    void splitFile(
        const std::string & content,
        const std::string & path);
    void setTime(const std::string & path);
    void mainLoop(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_id,
        std::string & cur_text,
        EsmReader::SubRecord & subrecord);
    void handleException(const std::exception & e);

    std::vector<Tools::Record> records;
    Tools::Name name;
    std::time_t time = 0;
    bool is_loaded = false;

    Tools::Record * rec = nullptr;

    SubRecord key;
    SubRecord value;
};

#endif // ESMREADER_HPP
