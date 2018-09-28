#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "config.hpp"
#include "dictreader.hpp"

class DictMerger
{
public:
    std::string getNamePrefix(size_t i) { return dict_coll[i].getNamePrefix(); }

    yampt::dict_t const& getDict() const { return dict; }
    yampt::single_dict_t const& getDict(yampt::rec_type type) const { return dict[type]; }
    yampt::single_dict_t const& getDict(size_t type) const { return dict[type]; }

    DictMerger();
    DictMerger(const std::vector<std::string> &path);

    void addRecord(const yampt::rec_type type,
                   const std::string &unique_text,
                   const std::string &friendly_text);

private:
    void mergeDict();
    void printSummaryLog();

    std::vector<DictReader> dict_coll;
    yampt::dict_t dict;

    int counter_merged = 0;
    int counter_replaced = 0;
    int counter_identical = 0;
    int counter_all = 0;
};

#endif // DICTMERGER_HPP
