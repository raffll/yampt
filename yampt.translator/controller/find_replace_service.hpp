#pragma once

#include "../model/row_provider.hpp"
#include "../model/dict_document.hpp"

#include <QString>
#include <optional>
#include <regex>
#include <string>

class find_replace_service_t
{
public:
	struct find_result_t
	{
		int row = -1;
		bool found = false;
	};

	struct replace_result_t
	{
		bool replaced = false;
		std::string new_text;
		std::string status;
	};

	struct replace_all_result_t
	{
		int count = 0;
	};

	find_replace_service_t(row_provider_t & provider, document_t *& active_doc);

	find_result_t find_next(const std::string & query, bool case_sensitive, bool regex_mode, int current_row);
	replace_result_t replace_current(
	    const std::string & query,
	    const std::string & replacement,
	    bool case_sensitive,
	    bool regex_mode,
	    int current_row);
	replace_all_result_t replace_all(
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

	std::optional<search_params_t> build_search_params(const std::string & query, const std::string & replacement, bool case_sensitive, bool regex_mode);
	bool matches_query(const std::string & text_value, const search_params_t & params);
	std::optional<std::string> apply_replacement(const std::string & source_text, const search_params_t & params);

	row_provider_t & provider_;
	document_t *& active_doc_;
};
