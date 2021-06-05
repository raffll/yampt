#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "dictreader.hpp"

class DictMerger
{
public:
    const auto & getName(size_t i) { return readers[i].getName(); }
    const auto & getDict() const { return dict; }

    DictMerger();
    DictMerger(const std::vector<std::string> & paths);

    void addRecord(
        const Tools::RecType type,
        const std::string & key_text,
        const std::string & val_text);

private:
    void mergeDict();
    void findDuplicateValues(Tools::RecType type);
    void findUnusedINFO();
    void printSummaryLog();

    std::vector<DictReader> readers;
    Tools::Dict dict;

    int counter_merged = 0;
    int counter_replaced = 0;
    int counter_identical = 0;
    int counter_all = 0;
};

#endif // DICTMERGER_HPP
