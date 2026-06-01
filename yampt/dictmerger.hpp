#pragma once

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
        const tools_t::rec_type_t type,
        const std::string & key_text,
        const std::string & val_text);

private:
    void mergeDict();
    void findDuplicateValues(tools_t::rec_type_t type);
    void findUnusedINFO();
    void printSummaryLog();

    std::vector<DictReader> readers;
    tools_t::dict_t dict;

    int counter_merged = 0;
    int counter_replaced = 0;
    int counter_identical = 0;
    int counter_all = 0;
};
