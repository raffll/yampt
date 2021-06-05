#ifndef DICTREADER_HPP
#define DICTREADER_HPP

#include "includes.hpp"
#include "tools.hpp"

class DictReader
{
public:
    const auto & isLoaded() { return is_loaded; }
    const auto & getName() { return name; }
    const auto & getDict() const { return dict; }

    DictReader(const std::string & path);
    DictReader(const DictReader & that);
    DictReader & operator=(const DictReader & that);
    ~DictReader() = default;

private:
    void parseDict(
        const std::string & content,
        const std::string & path);
    void validateEntry(const Tools::Entry & entry);
    void insertRecord(const Tools::Entry & entry);
    void insertCELL(const Tools::Entry & entry);
    void insertRNAM(const Tools::Entry & entry);
    void insertFNAM(const Tools::Entry & entry);
    void insertINFO(const Tools::Entry & entry);
    void printSummaryLog();

    Tools::Name name;
    Tools::Dict dict;

    bool is_loaded = false;

    int counter_loaded = 0;
    int counter_invalid = 0;
    int counter_doubled = 0;
    int counter_all = 0;
};

#endif // DICTREADER_HPP
