#include "base_dict_manager.hpp"
#include "../yampt/dictmerger.hpp"
#include <algorithm>
#include <cmath>

void base_dict_manager_t::set_paths(const std::vector<std::string> & paths)
{
    paths_ = paths;
}

void base_dict_manager_t::reload()
{
    merged_dict_ = tools_t::initializeDict();

    if (paths_.empty())
        return;

    DictMerger merger(paths_);
    merged_dict_ = merger.getDict();
}

const tools_t::dict_t & base_dict_manager_t::get_merged_dict() const
{
    return merged_dict_;
}

auto_translate_result_t base_dict_manager_t::auto_translate_from_base(editor_state_t & state)
{
    auto_translate_result_t result;

    for (auto & [type, chapter] : state.get_user_dict())
    {
        auto base_it = merged_dict_.find(type);
        if (base_it == merged_dict_.end())
            continue;

        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            auto & rec = chapter.records[i];

            bool is_untranslated = rec.status.empty() || rec.status == tools_t::Status::untranslated;
            if (!is_untranslated)
                continue;

            const auto * base_entry = base_it->second.find(rec.key_text);
            if (base_entry == nullptr)
            {
                ++result.skipped_no_match;
                continue;
            }

            if (base_entry->old_text != rec.old_text && !rec.old_text.empty() && !base_entry->old_text.empty())
            {
                ++result.skipped_text_changed;
                continue;
            }

            rec.new_text = base_entry->new_text;
            rec.status = tools_t::Status::auto_identical;
            state.mark_modified(type, i);
            ++result.translated;
        }
    }

    return result;
}

auto_translate_result_t base_dict_manager_t::auto_translate_identical(editor_state_t & state)
{
    auto_translate_result_t result;

    std::unordered_map<std::string, std::string> translated_texts;

    for (const auto & [type, chapter] : state.get_user_dict())
    {
        for (const auto & rec : chapter.records)
        {
            bool is_translated = !rec.status.empty()
                              && rec.status != tools_t::Status::untranslated;
            if (!is_translated)
                continue;
            if (rec.old_text.empty())
                continue;

            translated_texts.emplace(rec.old_text, rec.new_text);
        }
    }

    for (auto & [type, chapter] : state.get_user_dict())
    {
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            auto & rec = chapter.records[i];

            bool is_untranslated = rec.status.empty() || rec.status == tools_t::Status::untranslated;
            if (!is_untranslated)
                continue;

            auto found = translated_texts.find(rec.old_text);
            if (found == translated_texts.end())
            {
                ++result.skipped_no_match;
                continue;
            }

            rec.new_text = found->second;
            rec.status = tools_t::Status::auto_identical;
            state.mark_modified(type, i);
            ++result.translated;
        }
    }

    return result;
}

auto_translate_result_t base_dict_manager_t::auto_translate_heuristic(editor_state_t & state)
{
    auto_translate_result_t result;

    std::vector<std::pair<std::string, std::string>> translated_pairs;

    for (const auto & [type, chapter] : state.get_user_dict())
    {
        for (const auto & rec : chapter.records)
        {
            bool is_translated = !rec.status.empty()
                              && rec.status != tools_t::Status::untranslated;
            if (!is_translated)
                continue;
            if (rec.old_text.empty())
                continue;

            translated_pairs.emplace_back(rec.old_text, rec.new_text);
        }
    }

    for (auto & [type, chapter] : state.get_user_dict())
    {
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            auto & rec = chapter.records[i];

            bool is_untranslated = rec.status.empty() || rec.status == tools_t::Status::untranslated;
            if (!is_untranslated)
                continue;

            bool matched = false;
            for (const auto & [src_text, translated_text] : translated_pairs)
            {
                if (src_text == rec.old_text)
                    continue;

                if (!differs_only_in_numbers_or_punct(rec.old_text, src_text))
                    continue;

                std::string adapted = adapt_translation(rec.old_text, src_text, translated_text);
                rec.new_text = adapted;
                rec.status = tools_t::Status::auto_heuristic;
                state.mark_modified(type, i);
                ++result.translated;
                matched = true;
                break;
            }

            if (!matched)
                ++result.skipped_no_match;
        }
    }

    return result;
}

std::vector<fuzzy_match_t> base_dict_manager_t::find_fuzzy_matches(
    const std::string & text, tools_t::rec_type_t type,
    size_t max_results) const
{
    std::vector<fuzzy_match_t> matches;

    if (text.empty())
        return matches;

    auto it = merged_dict_.find(type);
    if (it == merged_dict_.end())
        return matches;

    for (const auto & entry : it->second.records)
    {
        if (entry.old_text.empty())
            continue;
        if (entry.old_text == text)
            continue;

        size_t max_len = std::max(text.size(), entry.old_text.size());
        size_t dist = levenshtein_distance(text, entry.old_text);
        float similarity = 1.0f - static_cast<float>(dist) / static_cast<float>(max_len);

        if (similarity < 0.5f)
            continue;

        fuzzy_match_t match;
        match.matched_key = entry.key_text;
        match.matched_value = entry.new_text;
        match.similarity = similarity;
        matches.push_back(match);
    }

    std::sort(matches.begin(), matches.end(),
              [](const fuzzy_match_t & a, const fuzzy_match_t & b)
              {
                  return a.similarity > b.similarity;
              });

    if (matches.size() > max_results)
        matches.resize(max_results);

    return matches;
}

void base_dict_manager_t::detect_changed(editor_state_t & state) const
{
    for (auto & [type, chapter] : state.get_user_dict())
    {
        auto base_it = merged_dict_.find(type);
        if (base_it == merged_dict_.end())
            continue;

        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            auto & rec = chapter.records[i];

            const auto * base_entry = base_it->second.find(rec.key_text);
            if (base_entry == nullptr)
                continue;

            if (base_entry->old_text.empty() || rec.old_text.empty())
                continue;

            if (base_entry->old_text == rec.old_text)
                continue;

            rec.status = tools_t::Status::changed;
            state.mark_modified(type, i);
        }
    }
}

size_t base_dict_manager_t::levenshtein_distance(const std::string & a, const std::string & b)
{
    size_t m = a.size();
    size_t n = b.size();

    if (m == 0)
        return n;
    if (n == 0)
        return m;

    std::vector<size_t> prev(n + 1);
    std::vector<size_t> curr(n + 1);

    for (size_t j = 0; j <= n; ++j)
        prev[j] = j;

    for (size_t i = 1; i <= m; ++i)
    {
        curr[0] = i;
        for (size_t j = 1; j <= n; ++j)
        {
            size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({curr[j - 1] + 1, prev[j] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, curr);
    }

    return prev[n];
}

static bool is_number_or_punct(char c)
{
    return (c >= '0' && c <= '9') || c == '.' || c == ',' || c == '-' || c == ':' || c == ';' || c == '!' || c == '?';
}

bool base_dict_manager_t::differs_only_in_numbers_or_punct(const std::string & a, const std::string & b)
{
    if (a.size() != b.size())
        return false;

    bool has_difference = false;

    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] == b[i])
            continue;

        if (!is_number_or_punct(a[i]) && !is_number_or_punct(b[i]))
            return false;

        has_difference = true;
    }

    return has_difference;
}

std::string base_dict_manager_t::adapt_translation(
    const std::string & source,
    const std::string & matched_source,
    const std::string & matched_translation)
{
    if (source.size() != matched_source.size())
        return matched_translation;

    std::string result = matched_translation;

    for (size_t i = 0; i < source.size() && i < matched_source.size(); ++i)
    {
        if (source[i] == matched_source[i])
            continue;

        auto pos = matched_translation.find(matched_source[i], 0);
        size_t search_start = 0;
        bool replaced = false;

        while (pos != std::string::npos)
        {
            bool is_in_diff_region = false;
            if (pos < matched_source.size() && matched_source[pos] != source[pos])
                is_in_diff_region = true;

            if (is_number_or_punct(result[pos]))
            {
                result[pos] = source[i];
                replaced = true;
                break;
            }

            search_start = pos + 1;
            pos = matched_translation.find(matched_source[i], search_start);
        }

        if (!replaced)
        {
            for (size_t j = 0; j < result.size(); ++j)
            {
                if (result[j] != matched_source[i])
                    continue;
                result[j] = source[i];
                break;
            }
        }
    }

    return result;
}
