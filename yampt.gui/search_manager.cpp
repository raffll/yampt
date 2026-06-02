#include "search_manager.hpp"
#include "editor_state.hpp"

static std::string to_lower(const std::string & str)
{
    std::string result = str;
    for (auto & c : result)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return result;
}

void search_manager_t::set_query(const std::string & text, bool case_sensitive)
{
    query_ = text;
    case_sensitive_ = case_sensitive;
    matches_.clear();
    current_ = 0;
}

void search_manager_t::find_all(const editor_state_t & state,
                                const std::set<tools_t::rec_type_t> & type_filter)
{
    matches_.clear();
    current_ = 0;

    if (query_.empty())
        return;

    const std::string search_term = case_sensitive_ ? query_ : to_lower(query_);
    const auto & dict = state.get_user_dict();

    for (const auto & [type, chapter] : dict)
    {
        if (!type_filter.empty() && type_filter.find(type) == type_filter.end())
            continue;

        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            const auto & entry = chapter.records[i];

            const std::string key = case_sensitive_ ? entry.key_text : to_lower(entry.key_text);
            size_t pos = 0;
            while ((pos = key.find(search_term, pos)) != std::string::npos)
            {
                search_match_t match;
                match.type = type;
                match.record_index = i;
                match.char_start = pos;
                match.char_end = pos + search_term.size();
                match.in_key = true;
                matches_.push_back(match);
                pos += search_term.size();
            }

            const std::string val = case_sensitive_ ? entry.new_text : to_lower(entry.new_text);
            pos = 0;
            while ((pos = val.find(search_term, pos)) != std::string::npos)
            {
                search_match_t match;
                match.type = type;
                match.record_index = i;
                match.char_start = pos;
                match.char_end = pos + search_term.size();
                match.in_key = false;
                matches_.push_back(match);
                pos += search_term.size();
            }
        }
    }
}

const search_match_t * search_manager_t::current_match() const
{
    if (matches_.empty())
        return nullptr;
    return &matches_[current_];
}

void search_manager_t::next_match()
{
    if (matches_.empty())
        return;
    current_ = (current_ + 1) % matches_.size();
}

void search_manager_t::prev_match()
{
    if (matches_.empty())
        return;
    current_ = (current_ == 0) ? matches_.size() - 1 : current_ - 1;
}

void search_manager_t::replace_current(editor_state_t & state, const std::string & replacement)
{
    if (matches_.empty())
        return;

    const auto & match = matches_[current_];
    if (match.in_key)
        return;

    auto & dict = state.get_user_dict();
    auto it = dict.find(match.type);
    if (it == dict.end())
        return;

    if (match.record_index >= it->second.records.size())
        return;

    auto & record = it->second.records[match.record_index];
    record.new_text.replace(match.char_start, match.char_end - match.char_start, replacement);
    state.mark_modified(match.type, match.record_index);
}

size_t search_manager_t::replace_all(editor_state_t & state, const std::string & replacement)
{
    size_t count = 0;
    auto & dict = state.get_user_dict();

    for (auto it = matches_.rbegin(); it != matches_.rend(); ++it)
    {
        if (it->in_key)
            continue;

        auto dict_it = dict.find(it->type);
        if (dict_it == dict.end())
            continue;

        if (it->record_index >= dict_it->second.records.size())
            continue;

        auto & record = dict_it->second.records[it->record_index];
        record.new_text.replace(it->char_start, it->char_end - it->char_start, replacement);
        state.mark_modified(it->type, it->record_index);
        ++count;
    }

    matches_.clear();
    current_ = 0;
    return count;
}

const std::vector<search_match_t> & search_manager_t::get_matches() const
{
    return matches_;
}

size_t search_manager_t::current_index() const
{
    return current_;
}
