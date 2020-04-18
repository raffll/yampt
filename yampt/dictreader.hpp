#ifndef DICTREADER_HPP
#define DICTREADER_HPP

#include "config.hpp"

class DictReader
{
public:
    bool isLoaded() { return is_loaded; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }

    Tools::dict_t const& getDict() const { return dict; }
    Tools::single_dict_t const& getDict(Tools::RecType type) const { return dict[type]; }
    Tools::single_dict_t const& getDict(size_t type) const { return dict[type]; }

    DictReader(const std::string &path);
    DictReader(const DictReader& that);
    DictReader& operator=(const DictReader& that);
    ~DictReader();

private:
    void parseDict(const std::string &content,
                   const std::string &path);
    void setName(const std::string &path);
    void validateRecord(const std::string &id,
                        const std::string &unique_text,
                        const std::string &friendly_text);
    void insertRecord(const Tools::RecType type,
                      const std::string &unique_text,
                      const std::string &friendly_text);
    void printWarningLog(const std::string &id,
                         const std::string &unique_text,
                         const std::string &comment);
    void printSummaryLog();

    std::string name_full;
    std::string name_prefix;
    Tools::dict_t dict;
    Tools tools;

    bool is_loaded;

    int counter_loaded = 0;
    int counter_invalid = 0;
    int counter_doubled = 0;
    int counter_all = 0;
};

#endif // DICTREADER_HPP
