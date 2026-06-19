#pragma once

#include <memory>
#include <string>
#include <vector>

struct spell_match_t
{
	size_t start;
	size_t end;
	std::string word;
};

class spell_checker_t
{
public:
	spell_checker_t();
	~spell_checker_t();

	spell_checker_t(spell_checker_t &&) noexcept;
	spell_checker_t & operator=(spell_checker_t &&) noexcept;

	bool load(const std::string & aff_path, const std::string & dic_path);
	bool is_loaded() const;

	bool check_word(const std::string & word) const;
	std::vector<std::string> suggest(const std::string & word) const;
	void add_to_user_dict(const std::string & word);

	std::vector<spell_match_t> find_misspelled(const std::string & text) const;

	void set_excluded_words(const std::vector<std::string> & words);

private:
	struct impl_t;
	std::unique_ptr<impl_t> impl_;
	std::vector<std::string> excluded_words_;
	std::vector<std::string> user_dict_words_;

	bool is_excluded(const std::string & word) const;
	bool is_mwscript_keyword(const std::string & word) const;
};
