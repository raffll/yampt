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
    void validateRecord();
    void insertRecord(
        const Tools::RecType type);
    void insertCELL();
    void insertRNAM();
    void insertFNAM();
    void insertINFO();
    void printSummaryLog();

    std::string name_full;
    std::string name_prefix;
    Tools::Dict dict;

    bool is_loaded = false;

    int counter_loaded = 0;
    int counter_invalid = 0;
    int counter_doubled = 0;
    int counter_all = 0;

    std::string type_name;
    std::string key_text;
    std::string val_text;
};

#endif // DICTREADER_HPP
