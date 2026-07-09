#include <catch2/catch_all.hpp>
#include <controller/editor_controller.hpp>
#include <io/dict_writer.hpp>
#include <model/dict_document.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>

namespace {

std::string create_test_dict(const dict_t & data)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_ps_test.json").string();
	path = string_utils::normalize_path(path);
	app_logger_t::reset_log();
	dict_writer_t::write(data, path);
	app_logger_t::reset_log();
	return path;
}

void cleanup_test_dict(const std::string & path)
{
	app_logger_t::reset_log();
	std::error_code error_code;
	std::filesystem::remove(path, error_code);
}

dict_t make_dict_with_entries()
{
	dict_t data;
	auto & chapter = data[rec_type_t::info];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Hello traveler";
	entry_a.new_text = "Hello traveler";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Hello traveler";
	entry_b.new_text = "Hello traveler";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	return data;
}

} // namespace

TEST_CASE("editor_controller_t::commit, pending model status applied", "[i][qt]")
{
	edit_history_t history;
	byte_limit_validator_t validation;
	glossary_t annotations;
	editor_controller_t controller(history, validation, annotations);

	const auto data = make_dict_with_entries();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::info;
	row.key_text = "key_a";
	row.old_text = "Hello traveler";
	row.new_text = "Hello traveler";
	row.status = status_t::untranslated;
	row.record_index = 0;

	controller.set_loaded_text(QString("Hello traveler"));
	controller.set_pending_status(status_t::model);
	const auto result = controller.commit(doc, row, "Witaj podrozniku");

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::model);

	cleanup_test_dict(path);
}

TEST_CASE("editor_controller_t::commit, pending status consumed after use", "[i][qt]")
{
	edit_history_t history;
	byte_limit_validator_t validation;
	glossary_t annotations;
	editor_controller_t controller(history, validation, annotations);

	const auto data = make_dict_with_entries();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row_a;
	row_a.type = rec_type_t::info;
	row_a.key_text = "key_a";
	row_a.old_text = "Hello traveler";
	row_a.new_text = "Hello traveler";
	row_a.status = status_t::untranslated;
	row_a.record_index = 0;

	table_row_t row_b;
	row_b.type = rec_type_t::info;
	row_b.key_text = "key_b";
	row_b.old_text = "Hello traveler";
	row_b.new_text = "Hello traveler";
	row_b.status = status_t::untranslated;
	row_b.record_index = 1;

	controller.set_loaded_text(QString("Hello traveler"));
	controller.set_pending_status(status_t::model);
	controller.commit(doc, row_a, "Witaj podrozniku");

	controller.set_loaded_text(QString("Hello traveler"));
	const auto result = controller.commit(doc, row_b, "Manually typed");

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::propagated);

	cleanup_test_dict(path);
}

TEST_CASE("editor_controller_t::commit, model status skips propagation", "[i][qt]")
{
	edit_history_t history;
	byte_limit_validator_t validation;
	glossary_t annotations;
	editor_controller_t controller(history, validation, annotations);

	const auto data = make_dict_with_entries();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::info;
	row.key_text = "key_a";
	row.old_text = "Hello traveler";
	row.new_text = "Hello traveler";
	row.status = status_t::untranslated;
	row.record_index = 0;

	controller.set_loaded_text(QString("Hello traveler"));
	controller.set_pending_status(status_t::model);
	const auto result = controller.commit(doc, row, "Witaj podrozniku");

	REQUIRE(result.propagated_count == 0);

	const auto & chapter = doc.data().at(rec_type_t::info);
	REQUIRE(chapter.records[1].status == status_t::untranslated);
	REQUIRE(chapter.records[1].new_text == "Hello traveler");

	cleanup_test_dict(path);
}

TEST_CASE("editor_controller_t::take_pending_status, returns nullopt when not set", "[u]")
{
	edit_history_t history;
	byte_limit_validator_t validation;
	glossary_t annotations;
	editor_controller_t controller(history, validation, annotations);

	const auto result = controller.take_pending_status();
	REQUIRE_FALSE(result.has_value());
}
