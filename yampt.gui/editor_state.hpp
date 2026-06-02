#pragma once

#include "../yampt/tools.hpp"
#include <set>
#include <string>
#include <vector>

struct record_view_t
{
    tools_t::rec_type_t type;
    size_t index;
    bool modified = false;
};

class editor_state_t
{
public:
    bool load_user_dict(const std::string & path);
    bool load_source_dict(const std::string & path);
    bool save_user_dict();
    bool save_user_dict_as(const std::string & path);

    tools_t::dict_t & get_user_dict();
    const tools_t::dict_t & get_user_dict() const;
    const tools_t::dict_t & get_source_dict() const;

    const std::string & get_user_path() const;
    bool has_unsaved_changes() const;
    void mark_modified(tools_t::rec_type_t type, size_t index);

    std::vector<record_view_t> get_filtered_records(
        const std::set<tools_t::rec_type_t> & type_filter,
        const std::set<std::string> & status_filter,
        const std::string & search_text,
        bool case_sensitive) const;

private:
    tools_t::dict_t user_dict_;
    tools_t::dict_t source_dict_;
    std::string user_path_;
    std::string source_path_;
    bool dirty_ = false;
    std::set<std::pair<tools_t::rec_type_t, size_t>> modified_records_;
};
