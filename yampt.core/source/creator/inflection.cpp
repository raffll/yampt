#include "inflection.hpp"
#include <hunspell/hunspell.hxx>
#include <set>
#include <sstream>

static constexpr int max_phrase_forms = 50;

struct inflection_t::impl_t
{
    std::unique_ptr<Hunspell> m_hunspell;
};

inflection_t::inflection_t()
    : m_impl(std::make_unique<impl_t>())
{
}

inflection_t::~inflection_t() = default;

bool inflection_t::load(const std::string & aff_path, const std::string & dic_path)
{
    try
    {
        m_impl->m_hunspell = std::make_unique<Hunspell>(aff_path.c_str(), dic_path.c_str());
        return m_impl->m_hunspell != nullptr;
    }
    catch (...)
    {
        m_impl->m_hunspell.reset();
        return false;
    }
}

bool inflection_t::is_loaded() const
{
    return m_impl->m_hunspell != nullptr;
}

static std::vector<std::string> generate_forms_for_word(
    Hunspell & hunspell,
    const std::string & word)
{
    const auto descriptions = hunspell.analyze(word);
    if (descriptions.empty())
        return {};

    std::set<std::string> unique_forms;
    for (const auto & description : descriptions)
    {
        const auto generated = hunspell.generate(word, description);
        for (const auto & form : generated)
        {
            if (form != word)
                unique_forms.insert(form);
        }
    }

    return {unique_forms.begin(), unique_forms.end()};
}

std::vector<std::string> inflection_t::word_forms(const std::string & word) const
{
    if (!is_loaded())
        return {};

    return generate_forms_for_word(*m_impl->m_hunspell, word);
}

static std::vector<std::string> split_by_space(const std::string & phrase)
{
    std::vector<std::string> words;
    std::istringstream stream(phrase);
    std::string token;
    while (stream >> token)
        words.push_back(token);

    return words;
}

static std::string join_words(const std::vector<std::string> & words)
{
    std::string result;
    for (size_t index = 0; index < words.size(); ++index)
    {
        if (index > 0)
            result += ' ';

        result += words[index];
    }
    return result;
}

static bool all_words_recognized(
    Hunspell & hunspell,
    const std::vector<std::string> & words)
{
    for (const auto & word : words)
    {
        if (!hunspell.spell(word))
            return false;
    }

    return true;
}

static std::vector<std::string> build_candidates_for_position(
    Hunspell & hunspell,
    const std::vector<std::string> & words,
    size_t position)
{
    const auto forms = generate_forms_for_word(hunspell, words[position]);

    std::vector<std::string> candidates;
    candidates.reserve(forms.size());

    for (const auto & form : forms)
    {
        auto modified_words = words;
        modified_words[position] = form;

        if (!all_words_recognized(hunspell, modified_words))
            continue;

        candidates.push_back(join_words(modified_words));
    }

    return candidates;
}

std::vector<std::string> inflection_t::phrase_forms(const std::string & phrase) const
{
    if (!is_loaded())
        return {};

    const auto words = split_by_space(phrase);
    if (words.empty())
        return {};

    if (words.size() == 1)
        return word_forms(phrase);

    std::set<std::string> unique_results;
    for (size_t position = 0; position < words.size(); ++position)
    {
        const auto candidates = build_candidates_for_position(
            *m_impl->m_hunspell, words, position);

        for (const auto & candidate : candidates)
        {
            if (candidate == phrase)
                continue;

            unique_results.insert(candidate);
            if (static_cast<int>(unique_results.size()) >= max_phrase_forms)
                return {unique_results.begin(), unique_results.end()};
        }
    }

    return {unique_results.begin(), unique_results.end()};
}
