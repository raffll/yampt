#include "inflection.hpp"
#include "phrase_form_builder.hpp"
#include "utility/app_logger.hpp"
#include <algorithm>
#include <hunspell/hunspell.hxx>
#include <set>
#include <sstream>

static constexpr int max_phrase_forms = 2000;

struct inflection_t::impl_t
{
	std::unique_ptr<Hunspell> m_hunspell;
};

inflection_t::inflection_t()
    : m_impl(std::make_unique<impl_t>())
{}

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

static std::vector<std::string> generate_forms_for_word(Hunspell & hunspell, const std::string & word)
{
	const auto stems = hunspell.stem(word);

	std::string debug_stems;
	for (const auto & stem : stems)
		debug_stems += stem + ",";

	app_logger_t::add_log(
	    "[debug] word=\"" + word + "\" spell=" + std::to_string(hunspell.spell(word)) + " stems=[" + debug_stems +
	    "]\r\n");

	std::set<std::string> unique_forms;
	for (const auto & stem : stems)
	{
		const auto expanded = hunspell.suffix_suggest(stem);
		app_logger_t::add_log(
		    "[debug]   suffix_suggest(stem=\"" + stem + "\") count=" + std::to_string(expanded.size()) + "\r\n");
		for (const auto & form : expanded)
		{
			if (form != word)
				unique_forms.insert(form);
		}
	}

	if (unique_forms.empty())
	{
		const auto expanded = hunspell.suffix_suggest(word);
		app_logger_t::add_log(
		    "[debug]   suffix_suggest(word=\"" + word + "\") count=" + std::to_string(expanded.size()) + "\r\n");
		for (const auto & form : expanded)
		{
			if (form != word)
				unique_forms.insert(form);
		}
	}

	return { unique_forms.begin(), unique_forms.end() };
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

static std::vector<std::string> build_word_candidates(Hunspell & hunspell, const std::string & word)
{
	std::vector<std::string> candidates{ word };

	const auto forms = generate_forms_for_word(hunspell, word);
	for (const auto & form : forms)
	{
		if (form != word)
			candidates.push_back(form);
	}

	return candidates;
}

static std::vector<std::vector<std::string>> build_per_word_candidates(
    Hunspell & hunspell,
    const std::vector<std::string> & words)
{
	std::vector<std::vector<std::string>> per_word_candidates;
	per_word_candidates.reserve(words.size());
	for (const auto & word : words)
		per_word_candidates.push_back(build_word_candidates(hunspell, word));

	return per_word_candidates;
}

static std::string join_with_space(const std::vector<std::string> & words)
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

std::vector<std::string> inflection_t::phrase_forms(const std::string & phrase) const
{
	if (!is_loaded())
		return {};

	const auto words = split_by_space(phrase);
	if (words.empty())
		return {};

	if (words.size() == 1)
		return word_forms(phrase);

	auto & hunspell = *m_impl->m_hunspell;
	const auto per_word_candidates = build_per_word_candidates(hunspell, words);

	const auto accept_all = [](const std::vector<std::string> &) { return true; };

	auto forms = phrase_form_builder::build_phrase_forms(per_word_candidates, accept_all, max_phrase_forms);

	const auto nominative_phrase = join_with_space(words);
	forms.erase(std::remove(forms.begin(), forms.end(), nominative_phrase), forms.end());

	return forms;
}
