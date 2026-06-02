#pragma once

#include "../yampt/tools.hpp"
#include "editor_state.hpp"
#include <string>
#include <vector>

struct auto_translate_result_t
{
    size_t translated = 0;
    size_t skipped_no_match = 0;
    size_t skipped_text_changed = 0;
};

struct fuzzy_match_t
{
    std::string matched_key;
    std::string matched_value;
    float similarity = 0.0f;
};

class base_dict_manager_t
{
public:
    void set_paths(const std::vector<std::string> & paths);
    void reload();
    const tools_t::dict_t & get_merged_dict() const;

    auto_translate_result_t auto_translate_from_base(editor_state_t & state);
    auto_translate_result_t auto_translate_identical(editor_state_t & state);
    auto_translate_result_t auto_translate_heuristic(editor_state_t & state);

    std::vector<fuzzy_match_t> find_fuzzy_matches(
        const std::string & text, tools_t::rec_type_t type,
        size_t max_results = 5) const;

    void detect_changed(editor_state_t & state) const;

private:
    std::vector<std::string> paths_;
    tools_t::dict_t merged_dict_;

    static size_t levenshtein_distance(const std::string & a, const std::string & b);
    static bool differs_only_in_numbers_or_punct(const std::string & a, const std::string & b);
    static std::string adapt_translation(const std::string & source, const std::string & matched_source, const std::string & matched_translation);
};
