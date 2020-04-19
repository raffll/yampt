#ifndef ESMREADER_HPP
#define ESMREADER_HPP

#include "config.hpp"

class EsmReader
{
public:
    void setRecordTo(size_t i);
    void replaceRecordContent(const std::string & new_rec);

    void setUniqueTo(const std::string & id);
    void setFriendlyTo(const std::string & id);
    void setNextFriendlyTo(const std::string & id);

    bool isLoaded() { return is_loaded; }
    std::string getNameFull() { return name_full; }
    std::string getNamePrefix() { return name_prefix; }
    std::string getNameSuffix() { return name_suffix; }
    std::time_t getTime() { return time; }
    std::vector<std::string> const & getRecords() const { return rec_coll; }

    std::string getRecordContent() { return *rec; }
    std::string getRecordId() { return rec_id; }

    std::string getUniqueWithNull() { return unique_text; }
    std::string getUniqueText() { return Tools::eraseNullChars(unique_text); }
    std::string getUniqueId() { return unique_id; }
    bool isUniqueValid() { return unique_exist; }

    std::string getFriendlyWithNull() { return friendly_text; }
    std::string getFriendlyText() { return Tools::eraseNullChars(friendly_text); }
    std::string getFriendlyId() { return friendly_id; }
    size_t getFriendlyPos() { return friendly_pos; }
    size_t getFriendlySize() { return friendly_size; }
    std::string getFriendlyCounter() { return std::to_string(friendly_counter); }
    bool isFriendlyValid() { return friendly_exist; }

    EsmReader() = default;
    EsmReader(const std::string & path);

private:
    void splitFileIntoRecordColl(
        const std::string & content,
        const std::string & path);
    void setName(const std::string & path);
    void setTime(const std::string & path);

    void uniqueMainLoop(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_id,
        std::string & cur_text);
    void caseForDialogType(
        std::size_t & cur_pos,
        std::string & cur_text);
    void caseForINDX(
        std::size_t & cur_pos,
        std::string & cur_text);
    void caseForDefault(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_text);
    void uniqueIfEndOfRecordReached(std::size_t & cur_pos);

    void friendlyMainLoop(
        std::size_t & cur_pos,
        std::size_t & cur_size,
        std::string & cur_id,
        std::string & cur_text);
    void friendlyIfEndOfRecordReached(std::size_t & cur_pos);

    void handleException(const std::exception & e);

    std::vector<std::string> rec_coll;
    std::string name_full;
    std::string name_prefix;
    std::string name_suffix;
    std::time_t time;
    bool is_loaded = false;

    std::string * rec = nullptr;
    size_t rec_size = 0;
    std::string rec_id;

    std::string unique_id;
    std::string unique_text;
    bool unique_exist = false;

    std::string friendly_id;
    std::string friendly_text;
    size_t friendly_pos = 0;
    size_t friendly_size = 0;
    size_t friendly_counter = 0;
    bool friendly_exist = false;
};

#endif // ESMREADER_HPP
