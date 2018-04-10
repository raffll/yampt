#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "config.hpp"

class EsmReader
{
public:
    void readFile(const std::string &path);
    void setRecordTo(size_t i);
    void setNewRecordContent(const std::string &new_rec);
    void setUniqueTo(const std::string id, bool erase_null = true);
    void setUniqueToINDX();
    void setUniqueToDialogType();
    void setFirstFriendlyTo(const std::string &id, bool erase_null = true);
    void setNextFriendlyTo(const std::string &id, bool erase_null = true);

    bool getStatus() { return status; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }
    std::vector<std::string> const& getRecordColl() const { return rec_coll; }

    std::string getRecordContent() { return *rec; }
    std::string getRecordId() { return rec_id; }

    std::string getUniqueText() { return unique_text; }
    std::string getUniqueId() { return unique_id; }
    bool getUniqueStatus() { return unique_status; }

    std::string getFriendlyText() { return friendly_text; }
    std::string getFriendlyId() { return friendly_id; }
    size_t getFriendlyPos() { return friendly_pos; }
    size_t getFriendlySize() { return friendly_size; }
    size_t getFriendlyCounter() { return friendly_counter; }
    bool getFriendlyStatus() { return friendly_status; }

    EsmReader();

private:
    void setName(const std::string &path);
    void setRecordColl(const std::string &content,
                       const std::string &path);

    Tools tools;
    std::vector<std::string> rec_coll;
    std::string name_full;
    std::string name_prefix;
    std::string name_suffix;

    bool status = false;

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

#endif // ESMREADER_HPP
