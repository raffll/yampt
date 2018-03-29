#ifndef DICTMERGER_HPP
#define DICTMERGER_HPP

#include "config.hpp"
#include "dictreader.hpp"

class DictMerger
{
public:
    void mergeDict();

    bool getStatus() { return status; }
    std::string getLog() { return log; }
    std::string getNamePrefix(size_t i) { return dict_coll[i].getNamePrefix(); }
    yampt::dict_t const& getDict() const { return dict; }
    yampt::inner_dict_t const& getDict(yampt::rec_type type) const { return dict[type]; }
    yampt::inner_dict_t const& getDict(size_t type) const { return dict[type]; }

    DictMerger();
    DictMerger(std::vector<std::string> &path);

private:
    void makeLog(const std::string &id,
                 const std::string &unique_text,
                 const std::string &friendly_old,
                 const std::string &friendly_new,
                 const std::string &comment,
                 const std::string &name);
    void printLog();

    std::vector<DictReader> dict_coll;
    yampt::dict_t dict;
    std::string log;

    bool status = false;

    int counter_merged = 0;
    int counter_replaced = 0;
    int counter_identical = 0;
    int counter_all = 0;
};

#endif // DICTMERGER_HPP
