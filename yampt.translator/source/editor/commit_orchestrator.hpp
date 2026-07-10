#pragma once

#include "byte_limit_validator.hpp"
#include "edit_history.hpp"
#include "glossary.hpp"
#include "../model/document.hpp"
#include "../model/table_row.hpp"
#include <string>

struct commit_input_t
{
	const table_row_t & row;
	const std::string & old_text;
	const std::string & new_text;
	status_t intent;
};

struct commit_output_t
{
	commit_result_t result;
	bool glossary_updated = false;
};

namespace commit_orchestrator {

commit_output_t execute(
    commit_input_t input,
    document_t & document,
    edit_history_t & history,
    byte_limit_validator_t & validator,
    glossary_t & glossary);

} // namespace commit_orchestrator
