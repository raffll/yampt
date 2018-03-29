#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "Config.hpp"

class EsmReader : public Tools
{
public:
    void readFile(const std::string &path);

    bool getStatus() { return status; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }

    std::vector<std::string> const& getRecColl() const { return rec_coll; }

    void setRecordTo(size_t i);
    void setNewRecordContent(const std::string &content) { *rec = content; }
    std::string getRecordContent() { return *rec; }
    std::string getRecordId() { return rec_id; }

    void setUniqueTo(const std::string id);
    std::string getUniqueText() { return unique_text = eraseNullChars(unique_text); }
    std::string getUniqueRaw() { return unique_text; }
    std::string getUniqueId() { return unique_id; }
    bool getUniqueStatus() { return unique_status; }

    void setFriendlyTo(const std::string id, const bool next = false);
    std::string getFriendlyText() { return friendly_text = eraseNullChars(friendly_text); }
    std::string getFriendlyRaw() { return friendly_text; }
    std::string getFriendlyId() { return friendly_id; }
    size_t getFriendlyPos() { return friendly_pos; }
    size_t getFriendlySize() { return friendly_size; }
    size_t getFriendlyCounter() { return friendly_counter; }
    bool getFriendlyStatus() { return friendly_status; }

    EsmReader();

private:
    void setName(const std::string &path);
    void setRecordColl(const std::string &content, const std::string &path);

    bool status = false;
    std::vector<std::string> rec_coll;

    std::string name_full;
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
