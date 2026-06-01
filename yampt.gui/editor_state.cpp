#include "editor_state.hpp"
#include "../yampt/dictreader.hpp"
#include "../yampt/dictwriter.hpp"

bool editor_state_t::load_user_dict(const std::string & path)
{
    DictReader reader(path);
    if (!reader.isLoaded())
        return false;

    user_dict_ = reader.getDict();
    user_path_ = path;
    dirty_ = false;
    modified_records_.clear();
    return true;
}

bool editor_state_t::load_source_dict(const std::string & path)
{
    DictReader reader(path);
    if (!reader.isLoaded())
        return false;

    source_dict_ = reader.getDict();
    source_path_ = path;
    return true;
}

bool editor_state_t::save_user_dict()
{
    if (user_path_.empty())
        return false;

    DictWriter::write(user_dict_, user_path_);
    dirty_ = false;
    modified_records_.clear();
    return true;
}

bool editor_state_t::save_user_dict_as(const std::string & path)
{
    DictWriter::write(user_dict_, path);
    user_path_ = path;
    dirty_ = false;
    modified_records_.clear();
    return true;
}

tools_t::dict_t & editor_state_t::get_user_dict()
{
    return user_dict_;
}

const tools_t::dict_t & editor_state_t::get_source_dict() const
{
    return source_dict_;
}

const std::string & editor_state_t::get_user_path() const
{
    return user_path_;
}

bool editor_state_t::has_unsaved_changes() const
{
    return dirty_;
}

void editor_state_t::mark_modified(tools_t::rec_type_t type, size_t index)
{
    dirty_ = true;
    modified_records_.insert({type, index});
}

std::vector<record_view_t> editor_state_t::get_filtered_records(
    const std::set<tools_t::rec_type_t> & type_filter,
    const std::set<std::string> & status_filter,
    const std::string & search_text,
    bool case_sensitive) const
{
    std::vector<record_view_t> result;

    for (const auto & [type, chapter] : user_dict_)
    {
        if (!type_filter.empty() && type_filter.find(type) == type_filter.end())
            continue;

        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            const auto & entry = chapter.records[i];

            if (!status_filter.empty() && status_filter.find(entry.status) == status_filter.end())
                continue;

            if (!search_text.empty())
            {
                bool found = false;
                if (case_sensitive)
                {
                    found = entry.key_text.find(search_text) != std::string::npos
                         || entry.new_text.find(search_text) != std::string::npos;
                }
                else
                {
                    std::string lower_search = search_text;
                    for (auto & c : lower_search)
                        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

                    std::string lower_key = entry.key_text;
                    for (auto & c : lower_key)
                        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

                    std::string lower_val = entry.new_text;
                    for (auto & c : lower_val)
                        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

                    found = lower_key.find(lower_search) != std::string::npos
                         || lower_val.find(lower_search) != std::string::npos;
                }

                if (!found)
                    continue;
            }

            record_view_t view;
            view.type = type;
            view.index = i;
            view.modified = modified_records_.count({type, i}) > 0;
            result.push_back(view);
        }
    }

    return result;
}
