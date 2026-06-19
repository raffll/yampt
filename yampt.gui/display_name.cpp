#include "display_name.hpp"

display_name_t::display_name_t(const std::string & filename)
    : filename_(filename)
{
}

void display_name_t::set_filename(const std::string & filename)
{
    filename_ = filename;
}

void display_name_t::set_kind(dict_kind_t kind)
{
    kind_ = kind;
}

void display_name_t::set_file_type(file_type_t type)
{
    file_type_ = type;
}

void display_name_t::set_language(const std::string & lang)
{
    language_ = lang;
}

void display_name_t::set_dirty(bool dirty)
{
    dirty_ = dirty;
}

void display_name_t::set_wip(bool wip)
{
    wip_ = wip;
}

std::string display_name_t::to_string() const
{
    std::string result;

    if (dirty_)
        result += "* ";

    if (kind_ == dict_kind_t::base)
        result += "[BASE] ";

    if (wip_)
        result += "[WIP] ";

    if (!language_.empty())
        result += "[" + language_ + "] ";

    result += filename_;
    return result;
}

const std::string & display_name_t::filename() const
{
    return filename_;
}
