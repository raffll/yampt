#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "Config.hpp"

class EsmReader : public Tools
{
public:
    void readFile(std::string path);

    bool getStatus() { return status; }
    std::string getName() { return name; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }

    std::vector<std::string> const& getRecColl() const { return rec_coll; }

    void setRec(size_t i);
    void setRecContent(std::string content) { *rec = content; }
    std::string getRecContent() { return *rec; }

    void setUnique(std::string id, bool erase_null = true);
    bool setFriendly(std::string id, bool erase_null = true, bool next = false);
    void setDump();

    std::string getRecId() { return rec_id; }

    std::string getUnique() { return unique_text; }
    std::string getUniqueId() { return unique_id; }
    bool getUniqueStatus() { return unique_status; }

    std::string getFriendly() { return friendly_text; }
    std::string getFriendlyId() { return friendly_id; }
    size_t getFriendlyPos() { return friendly_pos; }
    size_t getFriendlySize() { return friendly_size; }
    size_t getFriendlyCounter() { return friendly_counter; }
    bool getFriendlyStatus() { return friendly_status; }

    EsmReader();

private:
    void printStatus(std::string path);
    void setName(std::string path);
    void setRecColl(std::string &content);

    bool status = false;
    std::vector<std::string> rec_coll;

    std::string name;
    std::string name_prefix;
    std::string name_suffix;

    std::string *rec;

    size_t rec_size;
    std::string rec_id;

    std::string unique_id;
    std::string unique_text;
    bool unique_status = false;

    std::string friendly_id;
    std::string friendly_text;
    size_t friendly_pos;
    size_t friendly_size;
    size_t friendly_counter;
    bool friendly_status = false;
};

#endif
