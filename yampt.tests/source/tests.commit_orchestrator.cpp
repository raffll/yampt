#include <catch2/catch_all.hpp>
#include <editor/commit_orchestrator.hpp>
#include <io/dict_writer.hpp>
#include <model/dict_document.hpp>
#include <model/yaml_document.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>
#include <fstream>

namespace {

std::string create_dict(const dict_t & data)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_co_test.json").string();
	path = string_utils::normalize_path(path);
	app_logger_t::reset_log();
	dict_writer_t::write(data, path);
	app_logger_t::reset_log();
	return path;
}

void cleanup(const std::string & path)
{
	app_logger_t::reset_log();
	std::error_code error_code;
	std::filesystem::remove(path, error_code);
}

dict_t make_dict_with_entry(const std::string & old_text, status_t status)
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];
	record_entry_t entry;
	entry.key_text = "test_key";
	entry.old_text = old_text;
	entry.new_text = old_text;
	entry.status = status;
	chapter.records.push_back(std::move(entry));
	return data;
}

table_row_t make_row(const std::string & old_text, status_t status)
{
	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "test_key";
	row.old_text = old_text;
	row.new_text = old_text;
	row.status = status;
	row.record_index = 0;
	return row;
}

} // anonymous namespace

TEST_CASE("commit_orchestrator::execute, records history", "[i][qt]")
{
	auto data = make_dict_with_entry("Original", status_t::untranslated);
	const auto path = create_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	edit_history_t history;
	byte_limit_validator_t validator;
	glossary_t glossary;

	const auto row = make_row("Original", status_t::untranslated);
	const auto output = commit_orchestrator::execute(
	    { row, "Original", "Translated", status_t::in_progress },
	    doc, history, validator, glossary);

	REQUIRE(output.result.success);

	const auto entries = history.get_history(rec_type_t::cell, "test_key");
	REQUIRE(entries.size() == 1);
	REQUIRE(entries[0].value == "Original");

	cleanup(path);
}

TEST_CASE("commit_orchestrator::execute, validation overrides to error", "[i][qt]")
{
	auto data = make_dict_with_entry("Old", status_t::untranslated);
	const auto path = create_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	edit_history_t history;
	byte_limit_validator_t validator;
	validator.set_codepage(codepage_t::windows_1252);
	glossary_t glossary;

	const auto row = make_row("Old", status_t::untranslated);

	std::string long_text(600, 'A');
	const auto output = commit_orchestrator::execute(
	    { row, "Old", long_text, status_t::in_progress },
	    doc, history, validator, glossary);

	REQUIRE(output.result.success);
	REQUIRE(output.result.status == status_t::error);

	cleanup(path);
}

TEST_CASE("commit_orchestrator::execute, intent model preserved when valid", "[i][qt]")
{
	auto data = make_dict_with_entry("Hello", status_t::untranslated);
	const auto path = create_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	edit_history_t history;
	byte_limit_validator_t validator;
	glossary_t glossary;

	const auto row = make_row("Hello", status_t::untranslated);
	const auto output = commit_orchestrator::execute(
	    { row, "Hello", "Czesc", status_t::model },
	    doc, history, validator, glossary);

	REQUIRE(output.result.success);
	REQUIRE(output.result.status == status_t::model);

	cleanup(path);
}

TEST_CASE("commit_orchestrator::execute, updates glossary for dict", "[i][qt]")
{
	auto data = make_dict_with_entry("Sword", status_t::untranslated);
	auto & chapter = data[rec_type_t::fnam];
	record_entry_t fnam_entry;
	fnam_entry.key_text = "fnam_key";
	fnam_entry.old_text = "Sword";
	fnam_entry.new_text = "Sword";
	fnam_entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(fnam_entry));

	const auto path = create_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	edit_history_t history;
	byte_limit_validator_t validator;
	glossary_t glossary;

	table_row_t row;
	row.type = rec_type_t::fnam;
	row.key_text = "fnam_key";
	row.old_text = "Sword";
	row.new_text = "Sword";
	row.status = status_t::untranslated;
	row.record_index = 0;

	const auto output = commit_orchestrator::execute(
	    { row, "Sword", "Miecz", status_t::in_progress },
	    doc, history, validator, glossary);

	REQUIRE(output.result.success);
	REQUIRE(output.glossary_updated);

	cleanup(path);
}

TEST_CASE("commit_orchestrator::execute, does not update glossary for yaml", "[i][qt]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_co_yaml_test";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "greeting: Hello\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "";

	yaml_document_t doc(pl_path, "pl");

	edit_history_t history;
	byte_limit_validator_t validator;
	glossary_t glossary;

	table_row_t row;
	row.type = rec_type_t::yaml;
	row.key_text = "greeting";
	row.old_text = "Hello";
	row.new_text = "";
	row.status = status_t::untranslated;
	row.record_index = 0;

	const auto output = commit_orchestrator::execute(
	    { row, "", "Czesc", status_t::in_progress },
	    doc, history, validator, glossary);

	REQUIRE(output.result.success);
	REQUIRE_FALSE(output.glossary_updated);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("commit_orchestrator::execute, propagates for dict in_progress", "[i][qt]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Same";
	entry_a.new_text = "Same";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Same";
	entry_b.new_text = "Same";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	edit_history_t history;
	byte_limit_validator_t validator;
	glossary_t glossary;

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_a";
	row.old_text = "Same";
	row.new_text = "Same";
	row.status = status_t::untranslated;
	row.record_index = 0;

	const auto output = commit_orchestrator::execute(
	    { row, "Same", "New Value", status_t::in_progress },
	    doc, history, validator, glossary);

	REQUIRE(output.result.success);
	REQUIRE(output.result.propagated_count == 1);

	cleanup(path);
}

TEST_CASE("commit_orchestrator::execute, skips propagation for model intent", "[i][qt]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Same";
	entry_a.new_text = "Same";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Same";
	entry_b.new_text = "Same";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	edit_history_t history;
	byte_limit_validator_t validator;
	glossary_t glossary;

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_a";
	row.old_text = "Same";
	row.new_text = "Same";
	row.status = status_t::untranslated;
	row.record_index = 0;

	const auto output = commit_orchestrator::execute(
	    { row, "Same", "Model Result", status_t::model },
	    doc, history, validator, glossary);

	REQUIRE(output.result.success);
	REQUIRE(output.result.propagated_count == 0);

	cleanup(path);
}
