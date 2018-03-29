#ifndef DICTREADER_HPP
#define DICTREADER_HPP

#include "config.hpp"

class DictReader
{
public:
    void readFile(const std::string &path);

    bool getStatus() { return status; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getLog() { return log; }
    yampt::dict_t const& getDict() const { return dict; }
    yampt::inner_dict_t const& getDict(yampt::rec_type type) const { return dict[type]; }
    yampt::inner_dict_t const& getDict(size_t type) const { return dict[type]; }

    DictReader();
    DictReader(const DictReader& that);
    DictReader& operator=(const DictReader& that);
    ~DictReader();

private:
    void setName(const std::string &path);
    void parseDict(const std::string &content, const std::string &path);
    void validateRecord(const std::string &id, const std::string &unique_text, const std::string &friendly_text);
    void insertRecord(yampt::rec_type type, const std::string &unique_text, const std::string &friendly_text);
    void makeLog(const std::string &id, const std::string &unique_text, const std::string &friendly_text, const std::string &comment);
    void makeLogHeader();
    void printLog();

    std::string name_full;
    std::string name_prefix;
    yampt::dict_t dict;
    std::string log;

    bool status = false;

    int counter_loaded = 0;
    int counter_invalid = 0;
    int counter_doubled = 0;
    int counter_all = 0;
};

#endif // DICTREADER_HPP
