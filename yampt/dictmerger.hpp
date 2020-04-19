#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "includes.hpp"
#include "tools.hpp"
#include "dictreader.hpp"

class DictMerger
{
public:
    std::string getNamePrefix(size_t i) { return dict_coll[i].getNamePrefix(); }

    Tools::dict_t const & getDict() const { return dict; }
    Tools::single_dict_t const & getDict(Tools::RecType type) const { return dict[type]; }
    Tools::single_dict_t const & getDict(size_t type) const { return dict[type]; }

    DictMerger() = default;
    DictMerger(const std::vector<std::string> & path);

    void addRecord(
        const Tools::RecType type,
        const std::string & unique_text,
        const std::string & friendly_text);

private:
    void mergeDict();
    void findDuplicateFriendlyText(Tools::RecType type);
    void findUnusedINFO();
    void printSummaryLog();

    std::vector<DictReader> dict_coll;
    Tools::dict_t dict;
    bool ext_log;

    int counter_merged = 0;
    int counter_replaced = 0;
    int counter_identical = 0;
    int counter_all = 0;
};

#endif // DICTMERGER_HPP
