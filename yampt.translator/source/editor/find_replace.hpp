#pragma once

#include "../model/dict_document.hpp"
#include "../model/row_source.hpp"
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <QString>

class find_replace_t
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
		status_t status = status_t::untranslated;
	};

	struct replace_all_result_t
	{
		int count = 0;
	};

	struct undo_result_t
	{
		int count = 0;
	};

	find_replace_t(row_source_t & source, document_t *& active_doc);

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
	undo_result_t undo_last_replace_all();
	bool has_undo() const;

private:
	struct search_params_t
	{
		std::string query;
		std::string replacement;
		bool case_sensitive = false;
		std::optional<std::regex> regex_opt;
		QString lower_query;
	};

	struct undo_entry_t
	{
		rec_type_t type;
		size_t record_index;
		std::string old_text;
		status_t old_status;
	};

	std::optional<search_params_t> build_search_params(
	    const std::string & query,
	    const std::string & replacement,
	    bool case_sensitive,
	    bool regex_mode);
	bool matches_query(const std::string & text_value, const search_params_t & params);
	std::optional<std::string> apply_replacement(const std::string & source_text, const search_params_t & params);

	row_source_t & m_source;
	document_t *& m_active_doc;
	std::vector<undo_entry_t> m_undo_batch;
};
