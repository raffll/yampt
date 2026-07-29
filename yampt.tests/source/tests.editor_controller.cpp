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
	auto path = (fs::temp_directory_path() / "yampt_ec_test.json").string();
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

dict_t make_simple_dict()
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Old Cell";
	entry_a.new_text = "Old Cell";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Old Cell";
	entry_b.new_text = "Old Cell";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	record_entry_t entry_c;
	entry_c.key_text = "key_c";
	entry_c.old_text = "Different";
	entry_c.new_text = "Different";
	entry_c.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_c));

	return data;
}

} // anonymous namespace

TEST_CASE("dict_document_t::commit, matching row returns success", "[i]")
{
	const auto data = make_simple_dict();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_c";
	row.old_text = "Different";
	row.new_text = "Different";
	row.status = status_t::untranslated;
	row.record_index = 2;

	const auto result = doc.commit(row, "New Translation", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::in_progress);

	cleanup_test_dict(path);
}

TEST_CASE("dict_document_t::commit, invalid record_index returns not success", "[i]")
{
	const auto data = make_simple_dict();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "nonexistent";
	row.old_text = "x";
	row.new_text = "x";
	row.status = status_t::untranslated;
	row.record_index = 999;

	const auto result = doc.commit(row, "Anything", status_t::in_progress);

	REQUIRE_FALSE(result.success);

	cleanup_test_dict(path);
}

TEST_CASE("dict_document_t::propagate, matches trimmed old_text", "[i]")
{
	const auto data = make_simple_dict();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	const auto count = doc.propagate("Old Cell", "Propagated Value");

	REQUIRE(count == 2);

	const auto & chapter = doc.data().at(rec_type_t::cell);
	REQUIRE(chapter.records[0].new_text == "Propagated Value");
	REQUIRE(chapter.records[1].new_text == "Propagated Value");

	cleanup_test_dict(path);
}

TEST_CASE("dict_document_t::propagate, does not match different old_text", "[i]")
{
	const auto data = make_simple_dict();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	doc.propagate("Old Cell", "Propagated Value");

	const auto & chapter = doc.data().at(rec_type_t::cell);
	REQUIRE(chapter.records[2].new_text == "Different");

	cleanup_test_dict(path);
}

TEST_CASE("dict_document_t::propagate, sets status to propagated", "[i]")
{
	const auto data = make_simple_dict();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	doc.propagate("Old Cell", "New Value");

	const auto & chapter = doc.data().at(rec_type_t::cell);
	REQUIRE(chapter.records[0].status == status_t::propagated);
	REQUIRE(chapter.records[1].status == status_t::propagated);

	cleanup_test_dict(path);
}

TEST_CASE("dict_document_t::propagate, returns propagated count", "[i]")
{
	const auto data = make_simple_dict();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	const auto count = doc.propagate("Different", "Changed");

	REQUIRE(count == 1);

	cleanup_test_dict(path);
}

TEST_CASE("editor_controller_t::load, returns correct old and new text", "[i][qt]")
{
	glossary_t annotations;
	editor_controller_t controller(annotations);

	const auto data = make_simple_dict();
	const auto path = create_test_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_c";
	row.old_text = "Different";
	row.new_text = "Different";
	row.status = status_t::untranslated;
	row.record_index = 2;

	const auto result = controller.load(doc, row);

	REQUIRE(result.old_text == "Different");
	REQUIRE(result.new_text == "Different");

	cleanup_test_dict(path);
}
