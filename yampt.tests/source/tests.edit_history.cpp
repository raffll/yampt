#include <catch2/catch_all.hpp>
#include <editor/edit_history.hpp>
#include <rapidcheck/catch.h>
#include <utility/app_logger.hpp>
#include <utility/status_types.hpp>
#include <rapidcheck.h>

namespace {

rc::Gen<status_t> gen_status()
{
	return rc::gen::element(
	    status_t::translated,
	    status_t::untranslated,
	    status_t::in_progress,
	    status_t::model,
	    status_t::propagated,
	    status_t::adapted,
	    status_t::changed,
	    status_t::reused,
	    status_t::error);
}

rc::Gen<std::string> gen_edit_text()
{
	return rc::gen::nonEmpty(rc::gen::string<std::string>());
}

} // namespace

TEST_CASE("edit_history_t::revert, restores status at index N", "[u]")
{
	rc::prop(
	    "reverting to index N restores both text and status recorded at that point",
	    []()
	{
		const auto edit_count = *rc::gen::inRange(1, 20);
		const auto revert_index = *rc::gen::inRange(0, edit_count);

		edit_history_t history;
		const auto record_type = rec_type_t::info;
		const std::string record_key = "test_record_key";

		std::vector<std::string> recorded_values;
		std::vector<status_t> recorded_statuses;

		for (int index = 0; index < edit_count; ++index)
		{
			const auto old_value = *gen_edit_text();
			const auto new_value = *gen_edit_text();
			const auto old_status = *gen_status();

			history.record_change(record_type, record_key, old_value, new_value, old_status);
			recorded_values.push_back(old_value);
			recorded_statuses.push_back(old_status);
		}

		const auto result = history.revert(record_type, record_key, static_cast<size_t>(revert_index));

		RC_ASSERT(result.success);
		RC_ASSERT(result.reverted_text == recorded_values[static_cast<size_t>(revert_index)]);
		RC_ASSERT(result.reverted_status == recorded_statuses[static_cast<size_t>(revert_index)]);
	});
}
