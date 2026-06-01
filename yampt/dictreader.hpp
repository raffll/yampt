#pragma once

#include "includes.hpp"
#include "tools.hpp"

class DictReader
{
public:
    DictReader(const std::string & path);

    const auto & isLoaded() const { return is_loaded; }
    const auto & getName() const { return name; }
    const auto & getDict() const { return dict; }

private:
    void parseJson(const std::string & content, const std::string & path);
    void validateEntry(Tools::RecordEntry & entry, Tools::RecType type);

    Tools::Name name;
    Tools::Dict dict;
    bool is_loaded = false;
};
