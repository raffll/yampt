#include "commit_orchestrator.hpp"
#include <utility/app_logger.hpp>

commit_output_t commit_orchestrator::execute(
    commit_input_t input,
    document_t & document,
    edit_history_t & history,
    byte_limit_validator_t & validator,
    glossary_t & glossary)
{
	commit_output_t output;

	const auto validation = validator.validate(input.row.type, input.new_text);
	const auto effective_intent = (validation.level == validation_level_t::error) ? status_t::error : input.intent;

	if (validation.level == validation_level_t::error)
	{
		app_logger_t::add_log(
		    "[warning] status set to error for " + domain_types::type_to_str(input.row.type) + " \"" +
		    input.row.key_text + "\": " + validation.reason + "\r\n");
	}

	history.record_change(input.row.type, input.row.key_text, input.old_text, input.new_text, input.row.status);

	output.result = document.commit(input.row, input.new_text, effective_intent);
	if (!output.result.success)
		return output;

	if (document.kind() == document_kind_t::dict)
	{
		glossary.update_term(input.row.type, input.row.old_text, input.new_text);
		output.glossary_updated = true;
	}

	return output;
}
