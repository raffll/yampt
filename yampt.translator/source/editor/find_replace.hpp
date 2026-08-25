#pragma once

#include "../model/dict_document.hpp"
#include "../model/row_source.hpp"
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <QString>

class edit_history_t;

class find_replace_t
{
public:
	struct replace_all_result_t
	{
		int count = 0;
	};

	find_replace_t(row_source_t & source, document_t *& active_doc, edit_history_t & history);

	replace_all_result_t replace_all(
	    const std::string & query,
	    const std::string & replacement,
	    bool case_sensitive,
	    bool regex_mode);

	bool replace_one(
	    int row,
	    const std::string & query,
	    const std::string & replacement,
	    bool case_sensitive,
	    bool regex_mode);

private:
	struct search_params_t
	{
		std::string query;
		std::string replacement;
		bool case_sensitive = false;
		std::optional<std::regex> regex_opt;
		QString lower_query;
	};

	std::optional<search_params_t> build_search_params(
	    const std::string & query,
	    const std::string & replacement,
	    bool case_sensitive,
	    bool regex_mode);
	std::optional<std::string> apply_replacement(const std::string & source_text, const search_params_t & params);

	row_source_t & m_source;
	document_t *& m_active_doc;
	edit_history_t & m_history;
};
