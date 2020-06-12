#ifndef DICTREADER_HPP
#define DICTREADER_HPP

#include "includes.hpp"
#include "tools.hpp"

class DictReader
{
public:
    bool isLoaded() { return is_loaded; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    Tools::Dict const & getDict() const { return dict; }

    DictReader(const std::string & path);
    DictReader(const DictReader & that);
    DictReader & operator=(const DictReader & that);
    ~DictReader() = default;

private:
    void parseDict(
        const std::string & content,
        const std::string & path);
    void setName(const std::string & path);
    void validateRecord(
        const std::string & id,
        const std::string & unique_text,
        const std::string & friendly_text);
    void insertRecord(
        const Tools::RecType type,
        const std::string & unique_text,
        const std::string & friendly_text);
    void printSummaryLog();

    std::string name_full;
    std::string name_prefix;
    Tools::Dict dict;

    bool is_loaded = false;

    int counter_loaded = 0;
    int counter_invalid = 0;
    int counter_doubled = 0;
    int counter_all = 0;
};

#endif // DICTREADER_HPP
