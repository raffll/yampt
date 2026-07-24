#pragma once

#include <memory>
#include <string>
#include <vector>

class inflection_t
{
public:
    inflection_t();
    ~inflection_t();

    bool load(const std::string & aff_path, const std::string & dic_path);
    bool is_loaded() const;

    std::vector<std::string> word_forms(const std::string & word) const;
    std::vector<std::string> phrase_forms(const std::string & phrase) const;

private:
    struct impl_t;
    std::unique_ptr<impl_t> m_impl;
};
