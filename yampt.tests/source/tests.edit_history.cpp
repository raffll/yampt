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

TEST_CASE("edit_history_t::record_change, builds history list", "[u]")
{
	edit_history_t history;
	history.record_change(rec_type_t::cell, "Balmora", "old1", "new1", status_t::untranslated);
	history.record_change(rec_type_t::cell, "Balmora", "old2", "new2", status_t::in_progress);

	const auto entries = history.get_history(rec_type_t::cell, "Balmora");
	REQUIRE(entries.size() == 2);
	REQUIRE(entries[0].value == "old1");
	REQUIRE(entries[0].status == status_t::untranslated);
	REQUIRE(entries[1].value == "old2");
	REQUIRE(entries[1].status == status_t::in_progress);
}

TEST_CASE("edit_history_t::get_history, empty for unknown key", "[u]")
{
	edit_history_t history;
	const auto entries = history.get_history(rec_type_t::cell, "nonexistent");
	REQUIRE(entries.empty());
}

TEST_CASE("edit_history_t::get_history, separate keys independent", "[u]")
{
	edit_history_t history;
	history.record_change(rec_type_t::cell, "KeyA", "oldA", "newA", status_t::translated);
	history.record_change(rec_type_t::cell, "KeyB", "oldB", "newB", status_t::adapted);

	REQUIRE(history.get_history(rec_type_t::cell, "KeyA").size() == 1);
	REQUIRE(history.get_history(rec_type_t::cell, "KeyB").size() == 1);
	REQUIRE(history.get_history(rec_type_t::cell, "KeyA")[0].value == "oldA");
	REQUIRE(history.get_history(rec_type_t::cell, "KeyB")[0].value == "oldB");
}

TEST_CASE("edit_history_t::is_modified_this_session, tracks modifications", "[u]")
{
	edit_history_t history;
	REQUIRE(history.is_modified_this_session(rec_type_t::info, "key1") == false);

	history.record_change(rec_type_t::info, "key1", "old", "new", status_t::untranslated);
	REQUIRE(history.is_modified_this_session(rec_type_t::info, "key1") == true);
	REQUIRE(history.is_modified_this_session(rec_type_t::info, "key2") == false);
}

TEST_CASE("edit_history_t::revert, invalid index returns failure", "[u]")
{
	edit_history_t history;
	history.record_change(rec_type_t::cell, "key", "old", "new", status_t::translated);

	const auto result = history.revert(rec_type_t::cell, "key", 99);
	REQUIRE(result.success == false);
}

TEST_CASE("edit_history_t::revert, unknown key returns failure", "[u]")
{
	edit_history_t history;
	const auto result = history.revert(rec_type_t::cell, "unknown_key", 0);
	REQUIRE(result.success == false);
}

TEST_CASE("edit_history_t::save_to_file and load_from_file, round-trip", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_path = (fs::temp_directory_path() / "yampt_history_test.json").string();

	edit_history_t original;
	original.record_change(rec_type_t::cell, "Balmora", "old_value", "new_value", status_t::untranslated);
	original.record_change(rec_type_t::info, "dialogue_key", "hello", "czesc", status_t::translated);
	original.record_change(rec_type_t::cell, "Balmora", "new_value", "final_value", status_t::in_progress);

	original.save_to_file(temp_path);

	edit_history_t loaded;
	loaded.load_from_file(temp_path);

	const auto cell_history = loaded.get_history(rec_type_t::cell, "Balmora");
	REQUIRE(cell_history.size() == 2);
	REQUIRE(cell_history[0].value == "old_value");
	REQUIRE(cell_history[0].status == status_t::untranslated);
	REQUIRE(cell_history[1].value == "new_value");
	REQUIRE(cell_history[1].status == status_t::in_progress);

	const auto info_history = loaded.get_history(rec_type_t::info, "dialogue_key");
	REQUIRE(info_history.size() == 1);
	REQUIRE(info_history[0].value == "hello");
	REQUIRE(info_history[0].status == status_t::translated);

	std::error_code error_code;
	fs::remove(temp_path, error_code);
}

TEST_CASE("edit_history_t::load_from_file, missing file does nothing", "[i]")
{
	edit_history_t history;
	history.record_change(rec_type_t::cell, "key", "old", "new", status_t::untranslated);
	history.load_from_file("nonexistent_path_that_does_not_exist.json");

	REQUIRE(history.get_history(rec_type_t::cell, "key").empty());
}
