#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "dictreader.hpp"

class DictMerger
{
public:
    std::string getNamePrefix(size_t i) { return readers[i].getNamePrefix(); }
    Tools::Dict const & getDict() const { return dict; }

    DictMerger() = default;
    DictMerger(const std::vector<std::string> & paths);

    void addRecord(
        const Tools::RecType type,
        const std::string & key_text,
        const std::string & val_text);

private:
    void mergeDict();
    void findDuplicateFriendlyText(Tools::RecType type);
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
