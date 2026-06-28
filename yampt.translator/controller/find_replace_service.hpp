#pragma once

#include "../model/record_table_model.hpp"
#include "../model/dict_document.hpp"

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

	find_replace_service_t(record_table_model_t & model, document_t *& active_doc);

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
	record_table_model_t & model_;
	document_t *& active_doc_;
};
