#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "config.hpp"

class EsmReader
{
public:
    void setRecordTo(size_t i);
    void replaceRecordContent(const std::string &new_rec);

    void setUniqueTo(const std::string id,
                     const bool erase_null = true);
    void setFriendlyTo(const std::string &id,
                       const bool erase_null = true);
    void setNextFriendlyTo(const std::string &id,
                           const bool erase_null = true);

    bool isLoaded() { return is_loaded; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }
    std::time_t getTime() { return time; }
    std::vector<std::string> const& getRecordColl() const { return rec_coll; }

    std::string getRecordContent() { return *rec; }
    std::string getRecordId() { return rec_id; }

    std::string getUniqueText() { return unique_text; }
    std::string getUniqueId() { return unique_id; }
    bool isUniqueExist() { return unique_exist; }

    std::string getFriendlyText() { return friendly_text; }
    std::string getFriendlyId() { return friendly_id; }
    size_t getFriendlyPos() { return friendly_pos; }
    size_t getFriendlySize() { return friendly_size; }
    size_t getFriendlyCounter() { return friendly_counter; }
    bool isFriendlyExist() { return friendly_exist; }

    EsmReader();
    EsmReader(const std::string &path);

private:
    std::string readFile(const std::string &path);
    void splitFileIntoRecordColl(const std::string &content,
                                 const std::string &path);
    void setName(const std::string &path);
    void setTime(const std::string &path);

    void uniqueMainLoop(std::size_t &cur_pos,
                        std::size_t &cur_size,
                        std::string &cur_id,
                        std::string &cur_text,
                        bool erase_null);
    void caseForDialogType(std::size_t &cur_pos,
                           std::string &cur_text);
    void caseForINDX(std::size_t &cur_pos,
                     std::string &cur_text);
    void caseForCELL(std::size_t &cur_pos,
                     std::size_t &cur_size,
                     std::string &cur_text);
    void caseForDefault(std::size_t &cur_pos,
                        std::size_t &cur_size,
                        std::string &cur_text,
                        bool erase_null);
    void uniqueIfEndOfRecordReached(std::size_t &cur_pos);

    void friendlyMainLoop(std::size_t &cur_pos,
                          std::size_t &cur_size,
                          std::string &cur_id,
                          std::string &cur_text,
                          bool erase_null);
    void friendlyIfEndOfRecordReached(std::size_t &cur_pos);

    void handleException(const std::exception &e);

    Tools tools;
    std::vector<std::string> rec_coll;
    std::string name_full;
    std::string name_prefix;
    std::string name_suffix;
    std::time_t time;
    bool is_loaded;

    std::string *rec;
    size_t rec_size;
    std::string rec_id;

    std::string unique_id;
    std::string unique_text;
    bool unique_exist;

    std::string friendly_id;
    std::string friendly_text;
    size_t friendly_pos;
    size_t friendly_size;
    size_t friendly_counter;
    bool friendly_exist;
};

#endif // ESMREADER_HPP
