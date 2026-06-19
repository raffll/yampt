#pragma once

#include "../yampt/dict_kind.hpp"
#include "../yampt/file_list.hpp"

#include <string>

class display_name_t
{
public:
    display_name_t() = default;
    explicit display_name_t(const std::string & filename);

    void set_filename(const std::string & filename);
    void set_kind(dict_kind_t kind);
    void set_file_type(file_type_t type);
    void set_language(const std::string & lang);
    void set_dirty(bool dirty);

    std::string to_string() const;
    const std::string & filename() const;

private:
    std::string filename_;
    dict_kind_t kind_ = dict_kind_t::user;
    file_type_t file_type_ = file_type_t::user_dict;
    std::string language_;
    bool dirty_ = false;
};
