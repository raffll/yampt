#include "spell_checker.hpp"
#include "../../yampt/utility/tools.hpp"
#include <hunspell/hunspell.hxx>
#include <algorithm>
#include <cctype>

struct spell_checker_t::impl_t
{
	std::unique_ptr<Hunspell> hunspell;
};

spell_checker_t::spell_checker_t()
    : impl_(std::make_unique<impl_t>())
{}

spell_checker_t::~spell_checker_t() = default;

spell_checker_t::spell_checker_t(spell_checker_t &&) noexcept = default;
spell_checker_t & spell_checker_t::operator=(spell_checker_t &&) noexcept = default;

bool spell_checker_t::load(const std::string & aff_path, const std::string & dic_path)
{
	try
	{
		impl_->hunspell = std::make_unique<Hunspell>(aff_path.c_str(), dic_path.c_str());
	}
	catch (...)
	{
		impl_->hunspell.reset();
		return false;
	}
	return impl_->hunspell != nullptr;
}

bool spell_checker_t::is_loaded() const
{
	return impl_->hunspell != nullptr;
}

bool spell_checker_t::check_word(const std::string & word) const
{
	if (word.empty())
		return true;

	if (is_excluded(word))
		return true;

	if (is_mwscript_keyword(word))
		return true;

	if (!impl_->hunspell)
		return true;

	return impl_->hunspell->spell(word);
}

std::vector<std::string> spell_checker_t::suggest(const std::string & word) const
{
	if (!impl_->hunspell)
		return {};

	return impl_->hunspell->suggest(word);
}

void spell_checker_t::add_to_user_dict(const std::string & word)
{
	if (word.empty())
		return;

	user_dict_words_.push_back(word);

	if (impl_->hunspell)
		impl_->hunspell->add(word);
}

std::vector<spell_match_t> spell_checker_t::find_misspelled(const std::string & text) const
{
	std::vector<spell_match_t> results;

	if (!impl_->hunspell)
		return results;

	size_t i = 0;
	while (i < text.size())
	{
		unsigned char c = static_cast<unsigned char>(text[i]);

		bool is_word_char = std::isalnum(c) || c == '\'' || c >= 0x80;
		if (!is_word_char)
		{
			++i;
			continue;
		}

		size_t start = i;
		while (i < text.size())
		{
			unsigned char ch = static_cast<unsigned char>(text[i]);
			if (std::isalnum(ch) || ch == '\'' || ch >= 0x80)
				++i;
			else
				break;
		}

		size_t end = i;

		while (end > start && text[end - 1] == '\'')
			--end;

		if (end <= start)
			continue;

		std::string word = text.substr(start, end - start);

		if (!check_word(word))
			results.push_back({ start, end, word });
	}

	return results;
}

void spell_checker_t::set_excluded_words(const std::vector<std::string> & words)
{
	excluded_words_ = words;
}

bool spell_checker_t::is_excluded(const std::string & word) const
{
	for (const auto & excluded : excluded_words_)
	{
		if (excluded.size() != word.size())
			continue;

		bool match = true;
		for (size_t i = 0; i < word.size(); ++i)
		{
			if (std::tolower(static_cast<unsigned char>(excluded[i])) !=
			    std::tolower(static_cast<unsigned char>(word[i])))
			{
				match = false;
				break;
			}
		}

		if (match)
			return true;
	}

	return false;
}

bool spell_checker_t::is_mwscript_keyword(const std::string & word) const
{
	for (const auto & keyword : tools_t::keywords)
	{
		if (keyword.size() != word.size())
			continue;

		bool match = true;
		for (size_t i = 0; i < word.size(); ++i)
		{
			if (std::tolower(static_cast<unsigned char>(keyword[i])) !=
			    std::tolower(static_cast<unsigned char>(word[i])))
			{
				match = false;
				break;
			}
		}

		if (match)
			return true;
	}

	return false;
}
